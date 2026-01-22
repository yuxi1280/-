#ifndef __RELAY_CTRL_H_
#define __RELAY_CTRL_H_

#include "system_hc32f460.h"

// 继电器控制引脚宏定义（全部在PORTA）
#define RELAY_POS_ON()   (bCM_GPIO->POSRA_b.POS01 = 1)  // PA1置高：正极继电器接通
#define RELAY_POS_OFF()  (bCM_GPIO->POSRA_b.POS02 = 1)  // PA2置高：正极继电器断电
#define RELAY_NEG_ON()   (bCM_GPIO->POSRA_b.POS03 = 1)  // PA3置高：负极继电器接通
#define RELAY_NEG_OFF()  (bCM_GPIO->POSRA_b.POS04 = 1)  // PA4置高：负极继电器断电

#define RELAY_POS_RST()  (bCM_GPIO->PORRA_b.POR01 = 1)  // PA1清零
#define RELAY_POS2_RST() (bCM_GPIO->PORRA_b.POR02 = 1)  // PA2清零
#define RELAY_NEG_RST()  (bCM_GPIO->PORRA_b.POR03 = 1)  // PA3清零
#define RELAY_NEG2_RST() (bCM_GPIO->PORRA_b.POR04 = 1)  // PA4清零

typedef enum {
    RELAY_STATE_OFF = 0,
    RELAY_STATE_ON = 1
} relay_state_t;

void relay_ctrl_init(void);
void relay_connect(void);      // 接通充电线路（先负极后正极）
void relay_disconnect(void);   // 断开充电线路（先正极后负极）
relay_state_t relay_get_state(void);

#endif