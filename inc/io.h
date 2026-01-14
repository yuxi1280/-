#ifndef __IO_H_
#define __IO_H_

#include "system_hc32f460.h"

#define IO_RS485_RE_SET() (bCM_GPIO->POSRA_b.POS11=1)
#define IO_RS485_RE_RST() (bCM_GPIO->PORRA_b.POR11=1)
#define IO_RS485_TE_SET() (bCM_GPIO->POSRA_b.POS10=1)
#define IO_RS485_TE_RST() (bCM_GPIO->PORRA_b.POR10=1)

void io_init(void);

#endif

