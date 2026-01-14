#include "rs485.h"
#include "uart.h"
#include "doraemon_pack.h"
#include "c_queue.h"
#include "io.h"
#include "crc16_modbus.h"
#include "pid_speed.h"
#include "pid_pos.h"
#include "soft_timer.h"

#define RS485_TASK_QUEUE_BUF_MAX 64
#define RS485_TASK_QUEUE_MAX 3

static uint8_t id = 1;

void __mslex_usart_write(uint8_t *dat)
{
	// rs485_tx_str((char *)dat);
}

void _rx_execute(uint8_t *pkg, uint16_t len);

// c_queue_declare
typedef struct {
	uint8_t dat[RS485_TASK_QUEUE_BUF_MAX];
	uint16_t size;
}rx_buf_queue_t;

C_QUEUE_DECLARE(rx_buf_queue_t, RS485_TASK_QUEUE_MAX);

c_queue_rx_buf_queue_t_t rbq;
// c_queue_declare_end

static volatile uint64_t rx_frame_overtime = 0;

static uint8_t tx_dat_buf[UART_BUF_TX_MAX] = {0};
static uart_tx_buf_t rs485_tx_buf = {.head = 0, .tail = 0};
static uart_rx_buf_t rs485_rx_buf = {.head = 0, .tail = 0};

static uart_t rs485_uart;

static void __set_tx_mode(void)
{
	IO_RS485_RE_SET();
	IO_RS485_TE_SET();
}

static void __set_rx_mode(void)
{
	IO_RS485_TE_RST();
	IO_RS485_RE_RST();
}

// moto usart ti
void IRQ080_Handler(void)
{
	uint32_t head;

	if (bCM_USART1->CR1_b.TXEIE == 1 && rs485_uart.t_buf->head != rs485_uart.t_buf->tail) {
		head = rs485_uart.t_buf->head;
		CM_USART1->TDR = rs485_uart.t_buf->dat[head];
		rs485_uart.t_buf->head = BUF_NEXT(head, rs485_uart.t_buf->dat);
		if (rs485_uart.t_buf->head == rs485_uart.t_buf->tail) {
			bCM_USART1->CR1_b.TCIE = 1;
		}
	}
}

void IRQ083_Handler(void)
{
	if (rs485_uart.t_buf->head == rs485_uart.t_buf->tail) {
		bCM_USART1->CR1_b.TXEIE = 0;
		bCM_USART1->CR1_b.TCIE = 0;
		// while (bCM_USART1->SR_b.TC == 0) {}
		__set_rx_mode();
		// tx_start_flag = true;
	}
}

static void start_tx(void)
{
	if (bCM_USART1->CR1_b.TXEIE == 0) {
		__set_tx_mode();
		bCM_USART1->CR1_b.TE = 0;
		CM_USART1->CR1 |= 0x00000088ul;
	}
}

// moto usart ri
void IRQ081_Handler(void)
{
	uint32_t tail;
	uint8_t c;

	if (bCM_USART1->SR_b.FE == 1) {
		bCM_USART1->CR1_b.CFE = 1;
	}
	if (bCM_USART1->SR_b.RXNE == 1) {
		c = CM_USART1->RDR;
		tail = BUF_NEXT(rs485_uart.r_buf->tail, rs485_uart.r_buf->dat);
		if (tail == rs485_uart.r_buf->head) {
			return;
		} else {
			rs485_uart.r_buf->dat[rs485_uart.r_buf->tail] = c;
			rs485_uart.r_buf->tail = tail;
			// rx_frame_overtime = sys_get_tick();
		}
	}
}

// moto usart rto
void IRQ082_Handler(void)
{
	rx_buf_queue_t rq;
	uint16_t i;

	if (bCM_USART1->SR_b.RTOF == 1) {
		i = 0;
		while (!rs485_uart.r_buf_empty_check(&rs485_uart)) {
			rq.dat[i] = rs485_uart.r_buf->dat[rs485_uart.r_buf->head];
			rs485_uart.r_buf->head = BUF_NEXT(rs485_uart.r_buf->head, rs485_uart.r_buf->dat);
			i++;
		}
		rq.size = i;
		if (i > 0) {
			c_queue_rx_buf_queue_t_push(&rbq, &rq);
		}
		bCM_TMR0_2->BCONR_b.SYNSA = 0;
		bCM_TMR0_2->BCONR_b.CSTA = 0;
		bCM_TMR0_2->BCONR_b.SYNSA = 1;
		bCM_USART1->CR1_b.CRTOF = 1;
	}
}


static void hw_init(void)
{
	c_queue_rx_buf_queue_t_init(&rbq);

	//enable usart clock
	CM_PWC->FCG1 &= ~(1 << 24);
	// bCM_PWC->FCG2_b.TMR0_2 = 0;

	//config NVIC
	CM_INTC->SEL80 = 0x118ul;//USART_6_TI
	NVIC_SetPriority(INT080_IRQn, INT_PRIORITY_UART);
	NVIC_EnableIRQ(INT080_IRQn);
	CM_INTC->SEL81 = 0x117ul;//USART_6_RI
	NVIC_SetPriority(INT081_IRQn, INT_PRIORITY_UART);
	NVIC_EnableIRQ(INT081_IRQn);
	// CM_INTC->SEL82 = 0x175;
	// NVIC_SetPriority(INT082_IRQn, INT_PRIORITY_UART_OVERTIME);
	// NVIC_EnableIRQ(INT082_IRQn);
	CM_INTC->SEL83 = 0x119ul;
	NVIC_SetPriority(INT083_IRQn, INT_PRIORITY_UART_OVERTIME);
	NVIC_EnableIRQ(INT083_IRQn);

	//config usart
	CM_USART1->CR1 = 0x80000000;
	CM_USART1->CR2 = 0x00000600;
	CM_USART1->CR3 = 0x00000000;
	CM_USART1->PR = 0x00000000;

	// overtime setting
	// bCM_USART1->CR1_b.RTOIE = 1;
	// bCM_USART1->CR1_b.RTOE = 1;
	// bCM_TMR0_2->BCONR_b.CSTA = 0;
	// CM_TMR0_2->CNTAR = 0;
	// CM_TMR0_2->CMPAR = 100;//XXX
	// CM_TMR0_2->BCONR |= 0x00000000;
	// bCM_TMR0_2->BCONR_b.HSTAA = 1;
	// bCM_TMR0_2->BCONR_b.HCLEA = 1;
	// bCM_TMR0_2->BCONR_b.SYNSA = 1;
	// bCM_TMR0_2->BCONR_b.ASYNCLKA = 0;
	

	//baud = 115200
	CM_USART1->BRR = 0x0000357f;

	bCM_USART1->CR1_b.FBME = 1;
	bCM_USART1->CR1_b.M = 0;//UART_BIT_8
	bCM_USART1->CR2_b.STOP = 0;//UART_STOP_1_0
	bCM_USART1->CR1_b.PCE = 0;//是否校验
	bCM_USART1->CR1_b.TCIE = 0;
	bCM_USART1->CR1_b.RTOE = 0;
	bCM_USART1->CR1_b.RIE = 1;
	bCM_USART1->CR1_b.RE = 1;
	bCM_USART1->CR1_b.CFE = 1;
}

static void uart_suspend_handle(void)
{
	
}

void rs485_init(void)
{
	rs485_uart.baud = 115200;
	rs485_uart.bit = UART_BIT_8;
	rs485_uart.stop = UART_STOP_2_0;
	rs485_uart.t_buf = &rs485_tx_buf;
	rs485_uart.r_buf = &rs485_rx_buf;

	rs485_uart.start_tx = start_tx;
	rs485_uart.uart_suspend_handle = uart_suspend_handle;
	rs485_uart.hw_init = hw_init;

	uart_init(&rs485_uart);
}

// void debug_recv_exec(void *arg)
static void moto_recv_exec(void *arg)
{
}

void rs485_tx_str(char *dat)
{
	rs485_uart.buf_write_s(&rs485_uart, dat);
}

enum {
	RS485_RX_STATE_HEAD1 = 0,
	RS485_RX_STATE_HEAD2 = 1,
	RS485_RX_STATE_ID,
	RS485_RX_STATE_CMD,
	RS485_RX_STATE_LEN,
	RS485_RX_STATE_DATA,
	RS485_RX_STATE_CRCH,
	RS485_RX_STATE_CRCL,
};

void rs485_poll(void)
{
	static rx_buf_queue_t rq;
	uint32_t rx_len, pos;
	uint8_t rx_buf[RS485_TASK_QUEUE_BUF_MAX * 2];
	static uint16_t packet_len = 0;
	static uint16_t data_pos = 0;
	static uint16_t crc = 0;
	static uint8_t state = RS485_RX_STATE_HEAD1;
	static uint64_t overtime = 0, last;

	rx_len = 0;
	while (!rs485_uart.r_buf_empty_check(&rs485_uart)) {
		rx_buf[rx_len] = rs485_uart.r_buf->dat[rs485_uart.r_buf->head];
		rs485_uart.r_buf->head = BUF_NEXT(rs485_uart.r_buf->head, rs485_uart.r_buf->dat);
		rx_len++;
	}
	pos = 0;
	if (rx_len > 0) {
		for (uint32_t i = 0; i < rx_len; i++) {
			switch (state) {
			case RS485_RX_STATE_DATA:
				rq.dat[data_pos + 3] = rx_buf[i];
				data_pos++;
				if (packet_len == data_pos) {
					state = RS485_RX_STATE_CRCH;
					crc = 0;
				}
				break;
			case RS485_RX_STATE_HEAD1:
				if (rx_buf[i] == 0x39) {
					state = RS485_RX_STATE_HEAD2;
					data_pos = 0;
				}
				break;
			case RS485_RX_STATE_HEAD2:
				if (rx_buf[i] == 0x93) {
					state = RS485_RX_STATE_ID;
				} else {
					state = RS485_RX_STATE_HEAD1;
				}
				break;
			case RS485_RX_STATE_ID:
				rq.dat[0] = rx_buf[i];
				state = RS485_RX_STATE_CMD;
				break;
			case RS485_RX_STATE_CMD:
				rq.dat[1] = rx_buf[i];
				state = RS485_RX_STATE_LEN;
				break;
			case RS485_RX_STATE_LEN:
				rq.dat[2] = rx_buf[i];
				packet_len = rq.dat[2];
				if (packet_len == 0) {
					state = RS485_RX_STATE_CRCH;
				} else {
					state = RS485_RX_STATE_DATA;
				}
				break;
			case RS485_RX_STATE_CRCH:
				crc = rx_buf[i];
				state = RS485_RX_STATE_CRCL;
				break;
			case RS485_RX_STATE_CRCL:
				crc = crc << 8 | rx_buf[i];
				if (calc_modbus_crc16(rq.dat, rq.dat[2] + 3) == crc) {
					rq.size = rq.dat[2] + 3;
					c_queue_rx_buf_queue_t_push(&rbq, &rq);
				} else {
					__msl_printf("crc error");
					rq.size = 0;
					i = 0;
				}
				state = RS485_RX_STATE_HEAD1;
				break;
			default:
				state = RS485_RX_STATE_HEAD1;
				data_pos = 0;
				packet_len = 0;
				break;
			}
		}
		last = sys_get_tick();
	}
	while (!c_queue_rx_buf_queue_t_empty(&rbq)) {
		rx_buf_queue_t rq;
		c_queue_rx_buf_queue_t_front(&rbq, &rq);
		_rx_execute(rq.dat, rq.size);
		c_queue_rx_buf_queue_t_pop(&rbq);
	}
	overtime = sys_get_tick() - last;
	if (overtime >= 10000) {
		state = RS485_RX_STATE_HEAD1;
		data_pos = 0;
		packet_len = 0;
	}
	// static uint64_t now = 0;
	
	// if (sys_get_tick() - now >= 10000) {
	//         __msl_printf("working");
	//         now = sys_get_tick();
	// }
}

//-100.0
//39 93 01 01 04 c2 c8 00 00 97 47
//100.0
//39 93 01 01 04 42 c8 00 00 57 6e
void _rx_execute(uint8_t *pkg, uint16_t len)
{
	uint8_t tx_buf[20] = {0};
	uint16_t crc;

	if (pkg[0] == id) {
		switch (pkg[1]) {
		case RS485_CMD_SET_SPEED:
			// for (uint8_t i = 0; i < len; i++) {
			//         __msl_printf("%02x ", pkg[i]);
			// }
			if (pkg[2] == 4) {
				uint32_t tmp = 0;
				tmp = pkg[3];
				tmp = tmp << 8 | pkg[4];
				tmp = tmp << 8 | pkg[5];
				tmp = tmp << 8 | pkg[6];
				__msl_printf("%x", tmp);
				pid_speed_set_speed(*((float *)&tmp));
				// stimer_ctrl(2, true, STIMER_STATU_POS_ENABLE);
			}
			break;
		case RS485_CMD_SET_SPEED_PID:
			if (pkg[2] == 12) {
				uint32_t v_p = 0, v_d = 0, v_i = 0;
				v_p = pkg[3];
				v_p = v_p << 8 | pkg[4];
				v_p = v_p << 8 | pkg[5];
				v_p = v_p << 8 | pkg[6];
				v_i = pkg[7];
				v_i = v_i << 8 | pkg[8];
				v_i = v_i << 8 | pkg[9];
				v_i = v_i << 8 | pkg[10];
				v_d = pkg[11];
				v_d = v_d << 8 | pkg[12];
				v_d = v_d << 8 | pkg[13];
				v_d = v_d << 8 | pkg[14];
				__msl_printf("%x %x %x", v_p, v_i, v_d);
				pid_set_pid(*((float *)&v_p), *((float *)&v_i), *((float *)&v_d));
			}
			break;
		case RS485_CMD_SET_POS:
			if (pkg[2] == 4) {
				uint32_t tmp = 0;
				int32_t pos = 0;
				tmp = pkg[3];
				tmp = tmp << 8 | pkg[4];
				tmp = tmp << 8 | pkg[5];
				tmp = tmp << 8 | pkg[6];
				pos = *((int32_t *)&tmp);
				__msl_printf("%d\n", pos);
				pid_pos_set_pos((float)pos);

				tx_buf[0] = 0x39;
				tx_buf[1] = 0x93;
				tx_buf[2] = id;
				tx_buf[3] = RS485_CMD_SET_POS_RSP;
				tx_buf[4] = 0x00;
				crc = calc_modbus_crc16(tx_buf + 2, tx_buf[4] + 3); 
				tx_buf[5] = DP_UINT16_H(crc);
				tx_buf[6] = DP_UINT16_L(crc);
				rs485_uart.buf_write_n(&rs485_uart, tx_buf, 7);
			}
			break;
		case RS485_CMD_SET_POS_PID:
			if (pkg[2] == 12) {
				uint32_t v_p = 0, v_d = 0, v_i = 0;
				v_p = pkg[3];
				v_p = v_p << 8 | pkg[4];
				v_p = v_p << 8 | pkg[5];
				v_p = v_p << 8 | pkg[6];
				v_i = pkg[7];
				v_i = v_i << 8 | pkg[8];
				v_i = v_i << 8 | pkg[9];
				v_i = v_i << 8 | pkg[10];
				v_d = pkg[11];
				v_d = v_d << 8 | pkg[12];
				v_d = v_d << 8 | pkg[13];
				v_d = v_d << 8 | pkg[14];
				pid_pos_set_pid(*((float *)&v_p), *((float *)&v_i), *((float *)&v_d));

				tx_buf[0] = 0x39;
				tx_buf[1] = 0x93;
				tx_buf[2] = id;
				tx_buf[3] = RS485_CMD_SET_POS_PID_RSP;
				tx_buf[4] = 0x00;
				crc = calc_modbus_crc16(tx_buf + 2, tx_buf[4] + 3); 
				tx_buf[5] = DP_UINT16_H(crc);
				tx_buf[6] = DP_UINT16_L(crc);
				rs485_uart.buf_write_n(&rs485_uart, tx_buf, 7);
				__msl_printf("%x %x %x\n", v_p, v_i, v_d);
			}
			break;
		case RS485_CMD_GET_POS:
			if (pkg[2] == 0) {
				uint32_t pos;
				int32_t tmp;

				tmp = pid_pos_get_pos();
				pos = DP_GET_U32(tmp);
				tx_buf[0] = 0x39;
				tx_buf[1] = 0x93;
				tx_buf[2] = id;
				tx_buf[3] = RS485_CMD_GET_POS_RSP;
				tx_buf[4] = 0x04;
				tx_buf[5] = DP_UINT32_B3(pos);
				tx_buf[6] = DP_UINT32_B2(pos);
				tx_buf[7] = DP_UINT32_B1(pos);
				tx_buf[8] = DP_UINT32_B0(pos);
				crc = calc_modbus_crc16(tx_buf + 2, tx_buf[4] + 3); 
				tx_buf[9] = DP_UINT16_H(crc);
				tx_buf[10] = DP_UINT16_L(crc);
				rs485_uart.buf_write_n(&rs485_uart, tx_buf, 11);
				__msl_printf("pos:%d\n", pid_pos_get_pos());
			}
			break;
		case RS485_CMD_CALIBRATION:
			if (pkg[2] == 0) {
				uint32_t pos_max;
				int32_t tmp;

				crc = 0;
				pid_pos_calibration();
				tmp = pid_pos_get_pos_max();
				pos_max = DP_GET_U32(tmp);
				tx_buf[0] = 0x39;
				tx_buf[1] = 0x93;
				tx_buf[2] = id;
				tx_buf[3] = RS485_CMD_CALIBRATION_RSP;
				tx_buf[4] = 0x04;
				tx_buf[5] = DP_UINT32_B3(pos_max);
				tx_buf[6] = DP_UINT32_B2(pos_max);
				tx_buf[7] = DP_UINT32_B1(pos_max);
				tx_buf[8] = DP_UINT32_B0(pos_max);
				crc = calc_modbus_crc16(tx_buf + 2, tx_buf[4] + 3); 
				tx_buf[9] = DP_UINT16_H(crc);
				tx_buf[10] = DP_UINT16_L(crc);
				rs485_uart.buf_write_n(&rs485_uart, tx_buf, 11);
			}
			break;
		case RS485_CMD_GO_ZERO:
			if (pkg[2] == 0) {
				pid_pos_go_zero();
				tx_buf[0] = 0x39;
				tx_buf[1] = 0x93;
				tx_buf[2] = id;
				tx_buf[3] = RS485_CMD_GO_ZERO_RSP;
				tx_buf[4] = 0x00;
				crc = calc_modbus_crc16(tx_buf + 2, tx_buf[4] + 3); 
				tx_buf[5] = DP_UINT16_H(crc);
				tx_buf[6] = DP_UINT16_L(crc);
				rs485_uart.buf_write_n(&rs485_uart, tx_buf, 7);
			}
			break;
		case RS485_CMD_IDLE:
		default:
			// __msl_printf("idle\n");
			break;
		}
	} else {
		// __msl_printf("id error\n");
	}
}




