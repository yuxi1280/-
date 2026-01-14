#include "system_hc32f4a0pgtb.h"
#include "hc32f4a0pgtb.h"

/*******************************************************************************
 * Global pre-processor symbols/macros ('define')
 ******************************************************************************/
/**
 * @defgroup HC32F4A0_System_Local_Macros HC32F4A0 System Local Macros
 * @{
 */
#define HRC_16MHz_VALUE                 (16000000UL)  /*!< Internal high speed RC freq. */
#define HRC_20MHz_VALUE                 (20000000UL)  /*!< Internal high speed RC freq. */
/* HRC select */
#define HRC_FREQ_MON()                  (*((volatile uint32_t *)(0x40010684UL)))

/* Vector Table base offset field */
#ifndef VECT_TAB_OFFSET
#define VECT_TAB_OFFSET                 (0x0UL)     /*!< This value must be a multiple of 0x400. */
#endif
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

void sys_timer_init(void);

const uint32_t u32ICG[] __attribute__((section(".icg_sec"))) =
{
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,
    0xFFFFFFFFUL,

};

void sys_init(void)
{
    /* FPU settings */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
	SCB->CPACR |= ((3UL << 20) | (3UL << 22)); /* set CP10 and CP11 Full Access */
#endif
	sys_clk_init();
	/* Configure the Vector Table relocation */
	SCB->VTOR = VECT_TAB_OFFSET;    /* Vector Table Relocation */
}

void sys_clk_init(void)
{
	uint32_t tmp;

	CM_PWC->FPRC = 0xa50f;

	//reset reg
	CM_PWC->FCG0PC = 0xa5a50001;
	CM_PWC->FCG0 = 0xfffffa0e;
	CM_PWC->FCG0PC = 0xa5a50000;
	CM_PWC->FCG1 = 0xffffffff;
	CM_PWC->FCG2 = 0xffffffff;
	CM_PWC->FCG3 = 0xffffffff;
	sys_set_delay_clk(8000000);
	sys_delay_us(50);

	//config XTAL
	bCM_CMU->XTALCR_b.XTALSTP = 0x1;
	while (bCM_CMU->OSCSTBSR_b.XTALSTBF == 1) {}
	CM_CMU->XTALSTBCR = 0x05;//stable time;
	CM_CMU->XTALCFGR = 0x80;
	CM_CMU->XTALCFGR = 0xa0;//b6 xtalms: choose xtal mode 0:xtal 1:out_timer;
				//b5~b4 driver: 0:20~25MHz 1:16~20MHz 2:8~16MHz 3:4~8MHz;
				//b8 must be 1;
	bCM_CMU->XTALCR_b.XTALSTP = 0x0;
	while (bCM_CMU->OSCSTBSR_b.XTALSTBF == 0) {}

	//config pll
	CM_CMU->PLLHCR = 0x1;
	CM_CMU->PLLHCFGR = 0x22205900;//b31~b28: pllhp div: div=pllhp+1(pllhp can't be 0);
				      //b27~b24: pllhq div: same up;
				      //b23~b20: pllhr div: same up;
				      //b15~b8: pllhn 倍频系数: 系数=pllhn+1 24<=pllhn<=149;
				      //b7: clock source: 0:out xtal 1:inside rc;
				      //b1~b0: pllhm div: div=pllhm+1;
	CM_CMU->PLLHCR = 0x0;
	while (bCM_CMU->OSCSTBSR_b.PLLHSTBF == 0) {}

	//hclk=240MHz
	//pclk0=240MHz pclk1=120MHz pclk2=60MHz
	//pclk3=60MHz pclk4=120MHz exclk=120MHz
	CM_CMU->SCFGR = 0x00112210;//b26~b24: hclk div: div=2^value(value=[0:6]);
				   //b22~b20: exclk div: same up(up to 120MHz);
				   //b18~b16: pclk4 div: same up(up to 120MHz);
				   //b14~b12: pclk3 div: same up(up to 60MHz);
				   //b10~b8: pclk2 div: same up(up to 60MHz);
				   //b6~b4: pclk1 div: same up(up to 120MHz);
				   //b2~b0: pclk0 div: same up(up to 240MHz);
	CM_CMU->USBCKCFGR = 0x40;//b7~b4: usb clock: value 1~7 -> sysclk div 2~8
				 //8:PLLH/Q 9:PLLH/R 10:PLLA/P 11:PLLA/Q 12:PLLA/R;
	
	//config sram and flash wait time when read
	while ((CM_EFM->FRMC & 0x0000000f) != 5) {
		tmp = CM_EFM->FRMC;
		tmp &= 0x0000000f;
		tmp |= 0x00000005;
		CM_EFM->FAPRT = 0x0123;
		CM_EFM->FAPRT = 0x3210;
		CM_EFM->FRMC = tmp;//set flash wait time
				   //5:(200MHz,240MHz] 4:(160MHz,200MHz]
				   //3:(120MHz,160MHz] 2:(80MHz,120MHz]
				   //1:(40MHz,80MHz] 0:(,40MHz];
		sys_set_delay_clk(8000000);
		sys_delay_us(100);
	}
	CM_SRAMC->WTPR = 0x00000077;
	CM_SRAMC->WTCR = 0x11111111;//set sram wait period
	CM_SRAMC->WTPR = 0x00000076;

	//switch clock
	CM_CMU->CKSWR = 0x05;//b2~b0: 0:hrc 1:mrc 2:lrc 3:xtal 4:xtal32 5:pllh;
	sys_core_clk = 240000000;
	sys_set_delay_clk(240000000);
	sys_delay_us(50);

	//enable device in FCG0 reg
	CM_PWC->FCG0PC = 0xa5a50001;
	CM_PWC->FCG0 = 0xfef52a0e;//set clock for fcg0 device;
	CM_PWC->FCG0PC = 0xa5a50000;

	CM_PWC->FPRC = 0xa500;

	sys_nvic_init();
	sys_timer_init();
}

void sys_nvic_init(void)
{
	//config nvic
	NVIC_SetPriorityGrouping(0);
}

// only use when clk > 1M
uint32_t sys_delay_tick = 0;

void sys_set_delay_clk(uint32_t core_clk)
{
	sys_delay_tick = core_clk / 1000000 / 15;
}

void sys_delay_us(uint32_t usec)
{
	uint32_t t;

	t = (sys_delay_tick * usec);
	while (t--) {}
}

void IRQ038_Handler(void)
{
	if (bCM_DCU1->FLAG_b.FLAG_OP == 1) {
		CM_DCU1->DATA0 = 0;
		sys_tick_msb += 0x0000000100000000;
		bCM_DCU1->FLAGCLR_b.CLR_OP = 1;
	}
}

void sys_timer_init(void)
{
	CM_INTC->SEL38 = 0x37;//DCU1
	NVIC_SetPriority(INT038_IRQn, INT_PRIORITY_SYSTICK);
	NVIC_EnableIRQ(INT038_IRQn);

	//dcu power is open in sys_clk_init();
	bCM_PWC->FCG2_b.TMR0_1 = 0;

	CM_DCU1->DATA0 = 0xffff0000;
	CM_DCU1->DATA1 = 0x00000001;
	CM_DCU1->CTL = 0x80000023;//b31: interrupt enable, 0:disable 1:enable;
				  //b8: comptrg, compare trigger,
				  //0:after data0 write 1:after data0,1,2 write;
				  //b5~b4: data size, 0:8bit 1:16bit 2:32bit;
				  //b3~b0: mode, 0:dcu off 1:add when data1 write
				  //2:sub when data1 write 3:hardware add
				  //4:hardware sub 5:compare mode 8:trangle mode
				  //9:up sawtooth 10:down sawtooth
				  //** 8,9,10 only for dcu1,2,3,4;
	CM_DCU1->INTEVTSEL = 0x00000001;//interrupt enable
					//b11: trangle wave top, 0:off 1:on;
					//b10: trangle wave bottom, same top;
					//b8~b7: when mode is compare
					//0:decide ot b1~b6 
					//1:data2<=data0<=data1
					//2:data0>data1 or data0 < data2
					//3:off;
					//b6: 0:off 1:data0>data1;
					//b5: 0:off 1:data0==data1;
					//b4: 0:off 1:data0<data1;
					//b3: 0:off 1:data0>data2;
					//b2: 0:off 1:data0==data2;
					//b1: 0:off 1:data0<data2;
					//** b6~b1 only useful when b8~b7 is 0;
					//b0: 0:off 1:when add overflow or sub underflow;
					//** b11~b10 only for dcu1,2,3,4
	CM_AOS->DCU_TRGSEL1 = 0x00000060;//b31: dcu n+1 com trigger enable, 0:off 1:on;
					 //b30: dcu n com trigger enable, 0:off 1:on;
					 //** n is DCU_TRGSELn;
					 //b8~b0: event number, look in manual page 280;
					 //** this event whill trigger dcu(n) and dcu(n+1)

	CM_TMR0_1->CNTAR = 0;
	CM_TMR0_1->CMPAR = 373;//375;
	CM_TMR0_1->BCONR &= 0x00000000;
	CM_TMR0_1->BCONR |= 0x00004050;//b15: hardware capture trigger 0:off 1:on;
				       //b14: hardware clear cnt trigger 0:off 1:on;
				       //b13: hardware stop trigger 0:off 1:on;
				       //b12: hardware start trigger 0:off 1:on;
				       //b10: a channel asyn clock sel, 0:lRC 1:XTAL32;
				       //b9: a channel syn clock sel, 0:PCLK1 1:inside trigger;
				       //b8: a channel clock mode, 0:syn 1:asyn;
				       //b7~b4: clock div, n=[0,10] div=2^n;
				       //b2: a channel compare event interrupt, 0:off 1:on;
				       //b1: a channel mode, 0:compare output 0:input capture;
				       //b0: a channel enable, 0:off 1:on;
				       //** b31~b16 is for channel b, same to a;
	CM_AOS->TMR0_TRGSEL = 0x00000060;
	CM_TMR0_1->BCONR |= 0x00000001;
}

void sys_delay_ms(uint32_t ms)
{
	uint64_t now;
	uint64_t stop;

	stop = CM_DCU1->DATA0 + ms * 10;
	now = CM_DCU1->DATA0;
	while (now < stop) {
		now = CM_DCU1->DATA0;
	}
}

uint64_t sys_get_tick(void)
{
	return sys_tick_msb | CM_DCU1->DATA0;
}








