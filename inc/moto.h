#ifndef __MOTO_H_
#define __MOTO_H_

#include "system_hc32f460.h"

extern uint16_t moto_current_zero;

void moto_init(void);
uint16_t moto_get_current(void);
void moto_set_pwm(int16_t duty);

#endif
