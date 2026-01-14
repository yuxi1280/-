#ifndef __UART_H_
#define __UART_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define UART_BUF_LIST_MAX 20
#define UART_BUF_RX_MAX 512
#define UART_BUF_TX_MAX 256

#define BUF_NEXT(ptr, buf) ((ptr + 1) & (sizeof(buf) - 1))

enum {
	UART_BIT_8,
	UART_BIT_9
};

enum {
	UART_STOP_1_0,
	UART_STOP_1_5,
	UART_STOP_2_0
};

typedef struct {
	volatile char dat[UART_BUF_TX_MAX];
	volatile uint32_t head;
	volatile uint32_t tail;
}uart_tx_buf_t;

typedef struct {
	volatile char dat[UART_BUF_RX_MAX];
	volatile uint32_t head;
	volatile uint32_t tail;
}uart_rx_buf_t;

typedef struct uart_st{
	uint32_t baud;
	uint8_t bit;
	uint8_t stop;
	uart_tx_buf_t *t_buf;
	uart_rx_buf_t *r_buf;

	bool (*buf_write_1B_no_tx)(const struct uart_st *u, char c);
	bool (*buf_write_char)(const struct uart_st *u, char c);
	void (*buf_write_s)(const struct uart_st *u, const char *str);
	void (*buf_write_n)(const struct uart_st *u, const uint8_t *str, uint16_t n);
	uint16_t (*buf_read_c)(const struct uart_st *u);
	bool (*t_buf_empty_check)(const struct uart_st *u);
	bool (*r_buf_empty_check)(const struct uart_st *u);
	

	//set by user
	void (*start_tx)(void);
	void (*interrupt_handle)(void);
	void (*hw_init)(void);
	void (*uart_suspend_handle)(void);
}uart_t;

void uart_init(uart_t *u);

#endif


