#include "pid_pos.h"
#include "moto.h"
#include "encoder.h"

#define PID_POS_LIMIT_MAX (1000.0f)
#define PID_POS_LIMIT_MIN (-1000.0f)
#define PID_POS_MOTO_PWM_LIMIT (6000.0f)

static float _p = 1.2f, _i = 0.05f, _d = 1.1f;
static float e = 0.0f, e_i = 0.0f, e_d = 0.0f; //e_l1=e(k-1) e_l2=e(k-2)
static float pos = 0.0f, pos_dst = 0.0f;
static float pos_dst_min = 100.0f, pos_dst_max = 10000.0f;
static float i_limit_max = 0.0f, i_limit_min = 0.0f;
static float output = 0.0f;

int32_t pid_pos_get_pos_max(void)
{
	return (int32_t)pos_dst_max;
}	

void pid_pos_calibration(void)
{
	uint8_t count;
	int32_t e;

	pos_dst_max = 0;
	moto_set_pwm(10000);
	sys_delay_ms(1000);
	moto_set_pwm(-7000);
	count = 0;
	while (count < 5) { 
		if (encoder_get_e() == 0) {
			count++;
		}
		sys_delay_ms(10);
	}
	count = 0;
	e = 0;
	moto_set_pwm(10000);
	while (count < 5) {
		e = encoder_get_e();
		if (e == 0) {
			count++;
		} else {
			pos_dst_max += e;
		}
		sys_delay_ms(10);
		
	}
	moto_set_pwm(-10000);
	count = 0;
	while (count < 5) { 
		if (encoder_get_e() == 0) {
			count++;
		}
		sys_delay_ms(10);
	}
	encoder_get_e();
	sys_delay_ms(500);
	moto_set_pwm(0);
	pid_pos_set_pos(100.0f);
	__msl_printf("pid init done!\npos_dst_max: %d\n", (int32_t)pos_dst_max);
}

void pid_pos_go_zero(void)
{
	uint8_t count;
	int32_t e;

	moto_set_pwm(-10000);
	sys_delay_ms(1000);
	count = 0;
	while (count < 5) { 
		if (encoder_get_e() == 0) {
			count++;
		}
		sys_delay_ms(10);
	}
	moto_set_pwm(10000);
	sys_delay_ms(500);
	moto_set_pwm(-7000);
	count = 0;
	while (count < 5) { 
		if (encoder_get_e() == 0) {
			count++;
		}
		sys_delay_ms(10);
	}
	sys_delay_ms(500);
	moto_set_pwm(0);
	encoder_get_e();
	pid_pos_set_pos(0.0f);
	pos = 0.0f;
}

void pid_pos_init(void)
{
	e = 0.0f;
	e_i = 0.0f;
	e_d = 0.0f;
	pos = 0.0f;
	pos_dst = 0.0f;
	output = 0.0f;
	i_limit_max = PID_POS_LIMIT_MAX * 0.8f / _i;
	i_limit_min = PID_POS_LIMIT_MIN * 0.8f / _i;

	pid_pos_go_zero();
	pid_pos_set_pos(100.0f);
}

void pid_pos_set_pos(float p)
{
	if (p > pos_dst_max) {
		pos_dst = pos_dst_max;
	} else if (p < pos_dst_min) {
		pos_dst = pos_dst_min;
	} else {
		pos_dst = p;
	}
}

void pid_pos_update_pos(int32_t e)
{
	pos += e;
}

void pid_pos_set_pid(float v_p, float v_i, float v_d)
{
	_p = v_p;
	_i = v_i;
	i_limit_max = PID_POS_LIMIT_MAX * 0.8f / _i;
	i_limit_min = PID_POS_LIMIT_MIN * 0.8f / _i;
	_d = v_d;
}

int32_t pid_pos_get_pos(void)
{
	return (int32_t)pos;
}

float pid_pos_cal(void)
{
	e = pos_dst - pos;

	if ((e <= 100.0f && e >= 3.0f) || (e >= -100.0f && e <= -3.0f)) {
		e_i += e;
		if (e_i > i_limit_max) {
			e_i = i_limit_max;
		} else if (e_i < i_limit_min) {
			e_i = i_limit_min;
		}
	} else {
		e_i = 0;
	}
	output = _p * e + _i * e_i + _d * (e - e_d);
	// output = -output;
	e_d = e;
	if (e <= 3.0f && e >= -3.0f) {
		output = 0.0f;
	}

	if (output >= PID_POS_LIMIT_MAX) {
		output = PID_POS_LIMIT_MAX;
	} else if (output <= PID_POS_LIMIT_MIN) {
		output = PID_POS_LIMIT_MIN;
	}
	output *= 4.0f;

	if (output <= 0.5f && output >= -0.5f) {
		output = 0.0f;
	} else {
		if (output > 0.0f) {
			output += PID_POS_MOTO_PWM_LIMIT;
		} else {
			output -= PID_POS_MOTO_PWM_LIMIT;
		}
	}

	return output;
}



