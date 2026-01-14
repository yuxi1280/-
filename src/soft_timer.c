#include "soft_timer.h"

static stimer_t st_array[STIMER_NUM_MAX] = {0};
static uint32_t st_array_pos = 0;

stimer_t *stimer_get_by_id(uint16_t id)
{
	uint16_t i;

	for (i = 0; i < STIMER_NUM_MAX; i++) {
		if (st_array[i].id == id) {
			return &st_array[i];
		}
	}

	return NULL;
}

void stimer_clear(stimer_t *st)
{
	st->handle = NULL;
	st->old = 0;
	st->now = 0;
	st->period = 0;
	st->statu = 0;
}

void stimer_init(void)
{
	uint32_t i;

	for (i = 0; i < STIMER_NUM_MAX; i++) {
		st_array[i].id = STIMER_ID_MAX;
		stimer_clear(&st_array[i]);
	}
}

bool stimer_register(stimer_t *st)
{
	uint32_t i;

	for (i = 0; i < STIMER_NUM_MAX; i++) {
		if (st_array[i].id == STIMER_ID_MAX) {
			__msl_memcpy(&st_array[i], st, sizeof(stimer_t));
			return true;
		}
	}

	return false;
}

void stimer_del(uint16_t id)
{
	uint16_t i;
	stimer_t *st;

	st = stimer_get_by_id(id);
	if (st != NULL) {
		stimer_clear(st);
		st->id = STIMER_ID_MAX;
	}
}

void stimer_ctrl(uint16_t id, bool enable, uint8_t pos)
{
	stimer_t *st;

	st = stimer_get_by_id(id);
	st->old = sys_get_tick();
	if (st != NULL) {
		if (st->statu & STIMER_STATU_POS_LOCKED) {
			if (enable) {
				st->statu_waiting |= pos;
			} else {
				st->statu_waiting &= ~(pos);
			}
		} else {
			if (enable) {
				st->statu |= pos;
			} else {
				st->statu &= ~(pos);
			}
		}
	}
}

void stimer_set_period(uint16_t id, uint32_t period)
{
	stimer_t *st;

	st = stimer_get_by_id(id);
	if (st != NULL) {
		if (st_array[id].statu & STIMER_STATU_POS_LOCKED) {
			st_array[id].period_waiting = period;
		} else {
			st_array[id].period = period;
		}
	}
}

void stimer_poll(void)
{
	uint32_t i;
	stimer_t *st;

	for (i = 0; i < STIMER_NUM_MAX; i++) {
		st = &st_array[i];
		if (st->statu & STIMER_STATU_POS_ENABLE) {
			/*缓存当前状态进入影子备份*/
			st->statu_waiting = st->statu;
			/*锁住以免中断改变状态*/
			st->statu |= STIMER_STATU_POS_LOCKED;
			/*更新时间*/
			st->now = sys_get_tick();
			if (st->now - st->old >= st->period) {
				st->handle();
				st->old = st->now;
				if ((st->statu & STIMER_STATU_POS_PERIOD) == 0) {
					st->statu &= ~(STIMER_STATU_POS_ENABLE);
				}
			}
			/*解锁*/
			st->statu &= ~(STIMER_STATU_POS_LOCKED);
			if (st->statu & STIMER_STATU_POS_WAITING) {
				st->statu = st->statu_waiting;
				st->period = st->period_waiting;
				st->statu &= ~(STIMER_STATU_POS_WAITING);
			}
		}
	}
}



