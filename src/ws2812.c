#include "ws2812.h"

#define	WS2812_CODE0		0x8u
#define	WS2812_CODE1		0xeu

#define	WS2812_SPI_ON()		bCM_SPI1->CR1_b.SPE=0x1u
#define	WS2812_SPI_OFF()	bCM_SPI1->CR1_b.SPE=0x0u
#define WS2812_IF1_TDEF()	bCM_SPI1->SR_b.TDEF==0x1u	
#define WS2812_IF0_TDEF()	bCM_SPI1->SR_b.TDEF==0x0u
#define WS2812_RST()		ws2812_t_byte(0x00000000u)
#define WS2812_RSL()		ws2812_t_byte(0xffffffffu)

void ws2812_init(void)
{
	CM_PWC->FCG1 &= ~(1 << 16);
	CM_SPI1->CFG1 = 0x00000010;
	// bCM_SPI1->CFG2_b.LSBF		=0x0u;//高位是否在先选择位
	// bCM_SPI1->CFG2_b.DSIZE		=0xfu;//数据位数:0xf->32位
	// bCM_SPI1->CFG2_b.SSA		=0x0u;//cs选择
	// bCM_SPI1->CFG2_b.MBR		=0x4u;//预分频：2^(MBR+1)
	// bCM_SPI1->CFG2_b.CPOL		=0x1u;
	// bCM_SPI1->CFG2_b.CPHA		=0x1u;
	CM_SPI1->CFG2 = 0x00000f13;
	bCM_SPI1->CR1_b.MSTR = 0x1;//主从机模式选择：0从1主
	bCM_SPI1->CR1_b.TXMDS = 0x1;//0全双工，1仅发送
	WS2812_SPI_ON();
}

uint32_t ws2812_trans(uint8_t dat)
{
	uint8_t i;
	uint32_t result;

	result=0u;
	for(i=0; i < 8; i++)
	{
		result=result << 4;
		if(dat & 0x80)
			result += WS2812_CODE1;
		else
			result += WS2812_CODE0;
		dat=dat << 1;
	}
	return result;
}

void ws2812_t_byte(uint32_t dat)
{
	while(WS2812_IF0_TDEF()){}
	CM_SPI1->DR=dat;
}

void ws2812_reset(void)
{
	WS2812_RST();
	sys_delay_ms(1);
}

void ws2812_idle(void)
{
	WS2812_RSL();
}

void ws2812_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t trans[3];

	trans[0]=ws2812_trans(g);
	trans[1]=ws2812_trans(r);
	trans[2]=ws2812_trans(b);
	ws2812_t_byte(trans[0]);
	ws2812_t_byte(trans[1]);
	ws2812_t_byte(trans[2]);
	// WS2812_RSL();
}




