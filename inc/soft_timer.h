#ifndef __SOFT_TIMER_H_
#define __SOFT_TIMER_H_

#include "system_hc32f460.h"

#define STIMER_NUM_MAX 32

#define STIMER_ID_MAX 0xffffu

#define STIMER_STATU_POS_ENABLE 0x01u
#define STIMER_STATU_POS_PERIOD	0x02u
#define STIMER_STATU_POS_WAITING 0x40u
#define STIMER_STATU_POS_LOCKED 0x80u

/*waiting是定时器各项设置的影子备份，用于缓存对被锁存状态的定时器更改设置*/
typedef struct {
	uint16_t id;
	uint8_t statu;
	uint8_t statu_waiting;
	uint64_t old;
	uint64_t now;
	uint32_t period;
	uint32_t period_waiting;
	void (*handle)(void);
}stimer_t;

stimer_t *stimer_get_by_id(uint16_t id);
void stimer_clear(stimer_t *st);
void stimer_init(void);
bool stimer_register(stimer_t *st);
void stimer_del(uint16_t id);
void stimer_ctrl(uint16_t id, bool enable, uint8_t pos);
void stimer_set_period(uint16_t id, uint32_t period);
void stimer_poll(void);

#endif

