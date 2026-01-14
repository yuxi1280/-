#include "uart.h"

static bool buf_write_1B_no_tx(const uart_t *u, char c)
{
	uint32_t tail;

	tail = BUF_NEXT(u->t_buf->tail, u->t_buf->dat);
	while (u->t_buf->head == tail) {
		u->uart_suspend_handle();
	}
	u->t_buf->dat[u->t_buf->tail] = c;
	u->t_buf->tail = tail;

	return true;
}

static bool buf_write_char(const uart_t *u, char c)
{
	uint32_t tail;

	tail = BUF_NEXT(u->t_buf->tail, u->t_buf->dat);
	while (u->t_buf->head == tail) {
		u->uart_suspend_handle();
	}
	u->t_buf->dat[u->t_buf->tail] = c;
	u->t_buf->tail = tail;
	u->start_tx();

	return true;
}

static void buf_write_s(const uart_t *u, const char *str)
{
	char *p, c;

	p = (char *)str;
	while ((c = *p++) != '\0') {
		u->buf_write_1B_no_tx(u, c);
	}
	u->start_tx();
}

static void buf_write_n(const uart_t *u, const uint8_t *str, uint16_t n)
{
	uint8_t *p;

	p = (uint8_t *)str;
	while (n--) {
		u->buf_write_1B_no_tx(u, *p++);
	}
	u->start_tx();
}

static uint16_t buf_read_c(const uart_t *u)
{
	uint8_t c;

	if (u->r_buf->head == u->r_buf->tail) {
		return -1;
	} else {
		c = u->r_buf->dat[u->r_buf->head];
		u->r_buf->head = BUF_NEXT(u->r_buf->head, u->r_buf->dat);
		return c;
	}
}

static bool t_buf_empty_check(const uart_t *u)
{
	return (u->t_buf->head == u->t_buf->tail) ? true : false;
}

static bool r_buf_empty_check(const uart_t *u)
{
	return (u->r_buf->head == u->r_buf->tail) ? true : false;
}

//function for init uart with infomation in uart_t
void uart_init(uart_t *u)
{
	u->buf_write_1B_no_tx = buf_write_1B_no_tx;
	u->buf_write_char = buf_write_char;
	u->buf_write_s = buf_write_s;
	u->buf_write_n = buf_write_n;
	u->buf_read_c = buf_read_c;
	u->t_buf_empty_check = t_buf_empty_check;
	u->r_buf_empty_check = r_buf_empty_check;

	u->hw_init();
}



