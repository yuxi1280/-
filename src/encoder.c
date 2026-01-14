#include "encoder.h"

void encoder_init(void)
{
	CM_PWC->FCG2 &= ~(1 << 16);

	CM_TMR6_1->GCONR = 0x00000100u;
	CM_TMR6_1->PERAR = 0x0000ffffu;
	CM_TMR6_1->HCUPR = 0x00000069u;
	CM_TMR6_1->HCDOR = 0x00000096u;
	CM_TMR6_1->FCONR = 0x00000077u;
	// CM_TMR6_1->PCONR = 0x00010001u;
	CM_TMR6_1->CNTER = 0u;
	bCM_TMR6_1->GCONR_b.START = 0x1u;
}

uint16_t encoder_get(void)
{
	return CM_TMR6_1->CNTER;
}

int32_t encoder_get_e(void)
{
	static int32_t last_value = 0;
	int32_t value, e;

	value = (int32_t)CM_TMR6_1->CNTER;
	e = value - last_value;
	if (e > 32768) {
		e -= 65536;
	} else if (e < -32768) {
		e += 65536;
	}
	last_value = value;

	return e;
}

