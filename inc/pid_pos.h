#ifndef __PID_POS_H_
#define __PID_POS_H_

#include "system_hc32f460.h"

void pid_pos_init(void);
void pid_pos_set_pos(float p);
int32_t pid_pos_get_pos(void);
void pid_pos_update_pos(int32_t e);
void pid_pos_set_pid(float v_p, float v_i, float v_d);
float pid_pos_cal(void);
void pid_pos_calibration(void);
int32_t pid_pos_get_pos_max(void);
void pid_pos_go_zero(void);

#endif

