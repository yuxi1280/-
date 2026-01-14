#include "moto.h"

uint16_t moto_current_zero = 0;

uint16_t moto_get_current(void)
{
	CM_ADC1->STR = 0x01u;
	while ((CM_ADC1->ISR & 0x01u) == 0);
	return (uint16_t)CM_ADC1->DR0;
}

void moto_init(void)
{
	CM_PWC->FCG2 &= ~(1 << 7);

	CM_TMRA_6->BCSTRL = 0x02u;
	CM_TMRA_6->BCSTRH = 0x00u;
	CM_TMRA_6->CNTER = 0u;
	CM_TMRA_6->PERAR = 10000u;
	CM_TMRA_6->CMPAR7 = 0x1u;
	CM_TMRA_6->CMPAR8 = 0x1u;
	CM_TMRA_6->PCONR7 = 0x1243u;
	CM_TMRA_6->PCONR8 = 0x1243u;

	bCM_TMRA_6->BCSTRL_b.START = 1u;

	// CM_PWC->FCG3 &= ~(1 << 0);
	//
	// CM_ADC1->STR = 0x00u;
	// CM_ADC1->CR0 = 0x04c0u;//b10-b8：次数选择，2 ^ (AVCNT[2:0] + 1)
	//                        //b7：数据格式，0左对齐，1右对齐
	//                        //b6：自动清除数据寄存器，0禁止，1许可
	//                        //b5-b4：分辨率：0 12bit，1 10bit，2 8bit
	//                        //b1-b0：模式，0 序列A单次B无效，1 A连续B无效
	//                        //	2 A单次B单次， 3 A连续B单次
	// CM_ADC1->CR1 = 0x0000u;//b2：序列A重启模式，0 被B中断后从中断通道开始， 1 从第一个通道开始
	// CM_ADC1->TRGSR = 0x0000u;
	// CM_ADC1->CHSELRA = 0x00000001u;
	// CM_ADC1->AVCHSELR = 0x00000001u;
	// CM_ADC1->SSTR0 = 0xffu;
	// CM_ADC1->PGACR = 0x0000u;
	// CM_ADC1->PGAGSR = 0x000bu;
	// CM_ADC1->PGAINSR0 = 0x0001u;
	// CM_ADC1->PGAINSR1 = 0x0001u;
        //
	// moto_current_zero = 0;
	// for (uint8_t i = 0; i < 5; i++) {
	//         moto_current_zero += moto_get_current();
	// }
	// moto_current_zero /= 5;
	// __msl_printf("current zero: %d\n", moto_current_zero);
}

void moto_set_pwm(int16_t duty)
{
	if (duty == 0) {
		CM_TMRA_6->PCONR7 = 0x1243u;
		CM_TMRA_6->PCONR8 = 0x1243u;
		return;
	}
	if (duty >= 10000) {
		CM_TMRA_6->PCONR7 = 0x1343u;
		CM_TMRA_6->PCONR8 = 0x1243u;
		return;
	} else if (duty <= -10000) {
		CM_TMRA_6->PCONR7 = 0x1243u;
		CM_TMRA_6->PCONR8 = 0x1343u;
		return;
	}
	if (duty < 0) {
		CM_TMRA_6->CMPAR7 = 0x1u;
		CM_TMRA_6->CMPAR8 = -duty;
		CM_TMRA_6->PCONR7 = 0x1243u;
		CM_TMRA_6->PCONR8 = 0x1043u;
	} else {
		CM_TMRA_6->CMPAR7 = duty;
		CM_TMRA_6->CMPAR8 = 0x1u;
		CM_TMRA_6->PCONR7 = 0x1043u;
		CM_TMRA_6->PCONR8 = 0x1243u;
	}
}	


