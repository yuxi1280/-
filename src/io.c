#include "io.h"

void io_init(void)
{
	CM_GPIO->PWPR = 0xa501u;
	//global config
	CM_GPIO->PCCR &= 0x8fffu;
	CM_GPIO->PCCR |= 0xc000u;//b14~b12: RDWT set io read wait period
				//0:0MHz~50MHz 1:50MHz~100 2:100MHz~150MHz 
				//3:150MHz~200MHz 4:200MHz~250MHz
				//5,6,7:more than 250;
	CM_GPIO->PSPCR = 0x0003u;//b4: 0:NJTRST disable, 1:NJTRST enable;
				//b3: 0:JTDI disable, 1:JTDI enable;
				//b2: 0:JTDO_TRACESWO disable, 1:JTDO_TRACESWO enable;
				//b1: 0:JTMS_SWDIO disable, 1:JTMS_SWDIO enable;
				//b0: 0:JTCK_SWCLK disable, 1:JTCK_SWCLK enable;
	//led_595 io init 
	//sck:PB9 rck:PB8 dat:PB7
	// CM_GPIO->PORRB |= 0x0380u;
	// CM_GPIO->PCRB9 = 0x0050u;//b15: 数字功能 0有效 1关闭;
	//                         //b14: 输出锁存 0关闭 1有效;
	//                         //b12: 外部中断许可 0禁止 1有效;
	//                         //b10: 输入类型 0施密特输入 1cmos输入;
	//                         //b9: 反相输入输出 0禁止 1反相;
	//                         //b8: 输入数据 0低电平 1高电平 同PIDRx;
	//                         //b6: 上拉 0无上拉 1内部上拉;
	//                         //b5~b4: 驱动能力 0低驱动 1中驱动 2,3高驱动;
	//                         //b2: 开漏 0正常 1开漏;
	//                         //b1: 输出使能 0禁止 1使能 同POERx;
	//                         //b0: 输出数据 0低电平 1高电平;
	// CM_GPIO->PCRB8 = 0x0050u;
	// CM_GPIO->PCRB7 = 0x0050u;
	// CM_GPIO->POERB |= 0x0380u;//port b output enable;

	// rs485 io init
	// RX:PA12 /RE:PA11 TE:PA10 TX:PA9
	CM_GPIO->PCRA11 = 0x0052u;
	CM_GPIO->PCRA10 = 0x0052u;
	
	CM_GPIO->PFSRA9 = 32u;
	CM_GPIO->PCRA12 = 0x0050;
	CM_GPIO->PFSRA12 = 33u;

	// moto driver io init
	// moto_in1:PB5 moto_in2:PB6 moto_fault:PB4 moto_cnta:PA8 moto_cntb:PB13
	// moto_cnt_en:PB12 moto_adc:PA0
	CM_GPIO->PFSRB5 = 5u; //TIMA_6_PWM7
	CM_GPIO->PFSRB6 = 5u; //TIMA_6_PWM8
	CM_GPIO->PCRB4 = 0x0050u;
	
	CM_GPIO->PCRB12 = 0x0053u;
	CM_GPIO->PFSRB13 = 3u;
	CM_GPIO->PFSRA8 = 3u;

	CM_GPIO->PCRA0 = 0x8000;

	// at24c02 io init
	// wp:PA5 SDA:PA6 SCL:PA7
	CM_GPIO->PFSRA6 = 48u; //I2C1_SDA
	CM_GPIO->PFSRA7 = 49u; //I2C1_SCL

	CM_GPIO->PCRA5 = 0x0057u;

	CM_GPIO->PWPR = 0xa500u;
}


