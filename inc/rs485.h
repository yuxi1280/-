#ifndef __RS485_H_
#define __RS485_H_

#include "system_hc32f460.h"

enum {
	RS485_CMD_IDLE = 0,
	RS485_CMD_SET_SPEED = 0x01,
	RS485_CMD_SET_SPEED_PID = 0x02,
	RS485_CMD_SET_POS = 0x03,
	RS485_CMD_SET_POS_RSP = 0x83,
	RS485_CMD_SET_POS_PID = 0x04,
	RS485_CMD_SET_POS_PID_RSP = 0x84,
	RS485_CMD_GET_POS = 0x05,
	RS485_CMD_GET_POS_RSP = 0x85,
	RS485_CMD_SET_ID = 0x06,
	RS485_CMD_SET_ID_RSP = 0x86,
	RS485_CMD_CALIBRATION = 0x07,
	RS485_CMD_CALIBRATION_RSP = 0x87,
	RS485_CMD_GO_ZERO = 0x08,
	RS485_CMD_GO_ZERO_RSP = 0x88,
};

void rs485_init(void);
void rs485_tx_str(char *dat);
void rs485_poll(void);

#endif



