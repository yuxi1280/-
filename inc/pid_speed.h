#ifndef __PID_SPEED_H_
#define __PID_SPEED_H_

#include "system_hc32f460.h"

#define PID_SPEED_U_LIMIT_TOP 10000.0f
#define PID_SPEED_U_LIMIT_BOT -10000.0f

void pid_speed_init(void);
void pid_speed_set_speed(float spe);
float pid_speed_get_speed(void);
void pid_set_pid(float v_p, float v_i, float v_d);
float pid_speed_cal(float speed_now);

#endif

