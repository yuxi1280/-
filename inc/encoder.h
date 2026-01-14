#ifndef __ENCODER_H_
#define __ENCODER_H_

#include "system_hc32f460.h"

void encoder_init(void);
uint16_t encoder_get(void);
int32_t encoder_get_e(void);

#endif
