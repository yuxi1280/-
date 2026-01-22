#include "relay_ctrl.h"

static relay_state_t current_state = RELAY_STATE_OFF;

void relay_ctrl_init(void)
{
    // GPIO配置在io_init中完成
    relay_disconnect();
    current_state = RELAY_STATE_OFF;
    __msl_printf("Relay init done\n");
}

// 接通充电线路：先接通负极(PA3)，延迟后接通正极(PA1)
void relay_connect(void)
{
    if (current_state == RELAY_STATE_ON) {
        __msl_printf("Relay already ON\n");
        return;
    }
    
    /*
    // ===== 继电器控制代码（先注释，测试完成后取消注释） =====
    __msl_printf("Relay: Connecting NEG first (PA3)...\n");
    
    // 1. 先接通负极继电器 (PA3)
    RELAY_NEG_ON();
    sys_delay_ms(50);  // 给脉冲，确保自锁继电器吸合
    RELAY_NEG_RST();
    
    // 2. 延迟20ms后接通正极继电器 (PA1)
    sys_delay_ms(20);
    __msl_printf("Relay: Connecting POS (PA1)...\n");
    RELAY_POS_ON();
    sys_delay_ms(50);
    RELAY_POS_RST();
    
    sys_delay_ms(10);
    __msl_printf("Relay: Connected!\n");
    */
    
    __msl_printf("[TEST] Relay connect (code commented)\n");
    current_state = RELAY_STATE_ON;
}

// 断开充电线路：先断开正极(PA2)，再断开负极(PA4)
void relay_disconnect(void)
{
    /*
    // ===== 继电器控制代码（先注释，测试完成后取消注释） =====
    __msl_printf("Relay: Disconnecting POS first (PA2)...\n");
    
    // 1. 先断开正极继电器 (PA2)
    RELAY_POS_OFF();
    sys_delay_ms(50);
    RELAY_POS2_RST();
    
    // 2. 延迟20ms后断开负极继电器 (PA4)
    sys_delay_ms(20);
    __msl_printf("Relay: Disconnecting NEG (PA4)...\n");
    RELAY_NEG_OFF();
    sys_delay_ms(50);
    RELAY_NEG2_RST();
    
    sys_delay_ms(10);
    __msl_printf("Relay: Disconnected!\n");
    */
    
    __msl_printf("[TEST] Relay disconnect (code commented)\n");
    current_state = RELAY_STATE_OFF;
}

relay_state_t relay_get_state(void)
{
    return current_state;
}