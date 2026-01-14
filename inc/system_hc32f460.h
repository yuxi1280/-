#ifndef __SYSTEM_HC32F460_H__
#define __SYSTEM_HC32F460_H__

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include "hc32f460.h"
#include "core_cm4.h"
#include "mix_std_lib.h"

#define NVIC_VectTab_FLASH	(0x00000000)
#define AIRCR_VECTKEY_MASK	((uint32_t)0x05FA0000)
#define NVIC_PriorityGroup_0	((uint32_t)0x700) /*!< 0 bits for pre-emption priority 4 bits for subpriority */
#define NVIC_PriorityGroup_1	((uint32_t)0x600) /*!< 1 bits for pre-emption priority 3 bits for subpriority */
#define NVIC_PriorityGroup_2	((uint32_t)0x500) /*!< 2 bits for pre-emption priority 2 bits for subpriority */
#define NVIC_PriorityGroup_3	((uint32_t)0x400) /*!< 3 bits for pre-emption priority 1 bits for subpriority */
#define NVIC_PriorityGroup_4	((uint32_t)0x300) /*!< 4 bits for pre-emption priority 0 bits for subpriority */

enum {
	INT_PRIORITY_SYSTICK = 0,
	INT_PRIORITY_UART = 2,
	INT_PRIORITY_UART_OVERTIME = 2,
};

void WDT_init(void);
void WDT_flash(void);
void sys_init(void);
void sys_delay_ms(uint32_t ms);
uint64_t sys_get_tick(void);

#endif /* __SYSTEM_HC32F460_H__ */
