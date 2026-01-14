#include "at24c02.h"

//hw_part



void _hw_init(void)
{
	CM_PWC->FCG1 &= ~(1 << 4);
	
	CM_I2C1->CR1 = 0x00000040u;
	bCM_I2C1->CR1_b.PE = 0;
	bCM_I2C1->CR1_b.SWRST = 1;
	bCM_I2C1->CR1_b.PE = 1;
	CM_I2C1->SLR0 = 0x00000000u;
	CM_I2C1->SLR1 = 0x00000000u;
	// CM_I2C1->CCR =
}
//hw_part_end

void at24c02_init(void)
{
	_hw_init();
	
}


