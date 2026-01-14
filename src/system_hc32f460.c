#include "system_hc32f460.h"

#if defined ( __GNUC__ ) && !defined (__CC_ARM) /* GNU Compiler */
const uint32_t u32ICG[] __attribute__((section(".icg_sec"))) =
#elif defined (__CC_ARM)
const uint32_t u32ICG[] __attribute__((at(0x400))) =
#elif defined (__ICCARM__)
__root const uint32_t u32ICG[] @ 0x400 =
#else
#error "unsupported compiler!!"
#endif
{
	/* ICG 0~ 3 */
	0xfffffffful,
	0xfffffefful,
	0xfffffffful,
	0xfffffffful,

	/* ICG 4~ 7 */
	0xfffffffful,
	0xfffffffful,
	0xfffffffful,
	0xfffffffful,
};

#define HRC_16MHz_VALUE                 (16000000UL)  /*!< Internal high speed RC freq. */
#define HRC_20MHz_VALUE                 (20000000UL)  /*!< Internal high speed RC freq. */
/* HRC select */
#define HRC_FREQ_MON()                  (*((volatile uint32_t *)(0x40010684UL)))

/* Compiler Macros */
#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#ifndef __NO_INIT
#define __NO_INIT                       __attribute__((section(".bss.noinit")))
#endif /* __NO_INIT */
#elif defined ( __GNUC__ ) && !defined (__CC_ARM) /*!< GNU Compiler */
#ifndef __NO_INIT
#define __NO_INIT                       __attribute__((section(".noinit")))
#endif /* __NO_INIT */
#elif defined (__ICCARM__)              /*!< IAR Compiler */
#ifndef __NO_INIT
#define __NO_INIT                       __no_init
#endif /* __NO_INIT */
#elif defined (__CC_ARM)                /*!< ARM Compiler */
#ifndef __NO_INIT
#define __NO_INIT                       __attribute__((section(".bss.noinit"), zero_init))
#endif /* __NO_INIT */
#endif

/*!< System clock frequency (Core clock) */
__NO_INIT uint32_t sys_core_clk;
/*!< High speed RC frequency (HCR clock) */
__NO_INIT uint32_t HRC_VALUE;
volatile uint64_t sys_tick_msb = 0;

// only use when clk > 1M
uint32_t sys_delay_tick = 0;

void sys_set_delay_clk(uint32_t core_clk)
{
	sys_delay_tick = core_clk / 1000000 / 15;
}

void _sys_delay_us(uint32_t usec)
{
	uint32_t t;

	t = (sys_delay_tick * usec);
	while (t--) {}
}

void PORT_IN_init(void)
{
	CM_GPIO->PWPR = 0xa501u;
	CM_GPIO->PCCR |= 0xc000u;
	CM_GPIO->PSPCR = 0x0003;
	CM_GPIO->PWPR = 0xa500u;
}

void WDT_init(void)
{
	CM_WDT->CR = 0x800100b3u;
	/*看门狗设置
	bit31		中断请求:0中断，1复位
	bit16		sleep模式是否计数:0计数，1不计数
	bit11~bit8	刷新允许区域百分比:0%~100%（剩余设定参考手册）
	bit7~bit4	分频:这里选择div2048
	bit1~bit0	计数周期:这里是65536次+1
	*/
}

void WDT_flash(void)
{
	CM_WDT->RR = 0x00000123u;
	CM_WDT->RR = 0x00003210u;
}

void sys_clk_init(void)
{
	CM_PWC->FPRC = 0xa50fu;
	CM_PWC->FCG0PC = 0xa5a50001u;
	CM_PWC->FCG0 = 0xfefdfaee;//b31~b28:KEY,1,1,1
				  //b27~b24:DCU4,DCU3,DCU2,DCU1
				  //b23~b20:CRC,TRNG,HASH,AES
				  //b19~b16:1,1,AOS,FCM
				  //b15~b12:DMA2,DMA1,1,1
				  //b11~b08:1,SRAMRET,1,SRAM3
				  //b07~b04:1,1,1,SRAM12
				  //b03~b00:1,1,1,SRAMH
	CM_PWC->FCG0PC = 0xa5a50000u;
	CM_CMU->SCFGR = 0x00112211u;
	//对外设时钟分频进行设置：
	//time->MPLLP:200MHz
	//hclk=1,exclk=2div,pclk4=2div,pclk3=4div,pclk2=4div,pclk1=2div,pclk0=1div
	
	CM_CMU->XTALCR = 0x01u;
	CM_CMU->XTALCFGR = 0xa0u;
	CM_CMU->XTALCR = 0x00u;

	CM_SRAMC->WTPR = 0x77u;
	CM_SRAMC->WTCR = 0x11111111u;
	CM_SRAMC->WTPR = 0x76u;
	//配置sram的访问周期

	CM_EFM->FAPRT = 0x00000123u;
	CM_EFM->FAPRT = 0x00003210u;//解除flash寄存器保护
	bCM_EFM->FRMC_b.LVM = 0u;//清零超低功耗flash读取
	CM_EFM->FRMC &=  0xffffff0fu;
	CM_EFM->FRMC |=  0x00000050u;//更改flash读取等待周期：5个周期
	CM_EFM->FAPRT = 0x00002222u;//flash寄存器保护启动

	CM_PWC->PWRC2 = 0x0000000fu;
	CM_PWC->MDSWCR = 0x10u;
	sys_set_delay_clk(8000000);
	_sys_delay_us(1000);

	CM_CMU->PLLCR = 0x01u;
	CM_CMU->PLLCFGR = 0x11103100u;
	CM_CMU->PLLCR = 0x00u;
	//设置MPLL倍频时钟，时钟：8MHz * (49+1) = 400MHz
	//MPLLP:200MHz MPLLQ:200MHz MPLLR:200MHz
	
	while ((CM_CMU->OSCSTBSR & 0x00000028u) == 0){}
	
	CM_CMU->CKSWR = 0x05u;

	CM_PWC->FPRC = 0xa500u;
}

void IRQ038_Handler(void)
{
	if (bCM_DCU1->FLAG_b.FLAG_OP == 1) {
		sys_tick_msb += 0x0000000100000000u;
		bCM_DCU1->FLAGCLR_b.CLR_OP = 1;
	}
}

void sys_tick_init(void)
{
	CM_PWC->FCG2 &= 0xfffffffeu;

	CM_AOS->TMR0_TRGSEL = EVT_SRC_TMR0_1_CMP_A | 0x00000000u;
	CM_TMR0_1->CNTAR = 0x00000000u;
	CM_TMR0_1->CMPAR = 10000u;
	CM_TMR0_1->BCONR = 0x00004000u;
	
	CM_INTC->SEL38 = INT_SRC_DCU1;
	NVIC_SetPriority(INT038_IRQn, INT_PRIORITY_SYSTICK);
	NVIC_EnableIRQ(INT038_IRQn);
	CM_AOS->DCU_TRGSEL1 = EVT_SRC_TMR0_1_CMP_A | 0x00000000u;
	CM_DCU1->DATA0 = 0xffff0000u;
	CM_DCU1->DATA1 = 0x00000001u;
	CM_DCU1->CTL = 0x80000013u;
	CM_DCU1->INTEVTSEL = 0x00000001u;
	
	CM_TMR0_1->BCONR |= 0x00000001u;
}

void sys_init(void)
{
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
	SCB->CPACR |= ((3UL << 20) | (3UL << 22));
#endif

	sys_clk_init();
	SCB->VTOR = NVIC_VectTab_FLASH;
	NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);
	sys_tick_init();
}

void sys_delay_ms(uint32_t ms)
{
	uint64_t now;
	uint64_t e;
	
	now = sys_get_tick();
	e = ms * 10;
	for (;;) {
		if ((sys_get_tick() - now) >= e) {
			break;
		}
	}
}

uint64_t sys_get_tick(void)
{
	return sys_tick_msb | CM_DCU1->DATA0;
}


