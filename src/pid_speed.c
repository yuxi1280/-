#include "pid_speed.h"

//Δu(k) = Kp * [e(k) - e(k-1)] + Ki * e(k) + Kd * [e(k) - 2e(k-1) + e(k-2)]

static float _p = 100.0f, _i = 0.01f, _d = 0.0f;
static float e = 0.0f, e_l1 = 0.0f, e_l2 = 0.0f; //e_l1=e(k-1) e_l2=e(k-2)
static float spe_dst = 0.0f;
static float u = 0.0f, u_d = 0.0f;

void pid_speed_init(void)
{
	e = 0.0f;
	e_l1 = 0.0f;
	e_l2 = 0.0f;
	spe_dst = 0.0f;
	u_d = 0.0f;
	u = 0.0f;
}

void pid_speed_set_speed(float spe)
{
	spe_dst = spe;
}

float pid_speed_get_speed(void)
{
	return spe_dst;
}

void pid_set_pid(float v_p, float v_i, float v_d)
{
	_p = v_p;
	_i = v_i;
	_d = v_d;
}

float pid_speed_cal(float speed_now)
{
	float e_sub_e_l1;

	e = spe_dst - speed_now;
	e_sub_e_l1 = e - e_l1;

	u_d = _p * e_sub_e_l1 + _i * e + _d * (e_sub_e_l1 - e_l1 + e_l2);

	e_l2 = e_l1;
	e_l1 = e;

	u += u_d;

	if (u >= PID_SPEED_U_LIMIT_TOP) {
		u = PID_SPEED_U_LIMIT_TOP;
	} else if (u <= PID_SPEED_U_LIMIT_BOT) {
		u = PID_SPEED_U_LIMIT_BOT;
	}

	return u;
}

