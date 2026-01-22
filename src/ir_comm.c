#include "ir_comm.h"
#include "crc16_modbus.h"
#include "doraemon_pack.h"
#include "hc32f460.h"
#include "relay_ctrl.h"

// ========== 函数原型声明 ==========
static void ir_process_packet(uint8_t cmd, uint8_t *data, uint8_t len);

// ========== 配置参数 ==========
#define IR_PACKET_HEAD1 0x39
#define IR_PACKET_HEAD2 0x93
#define IR_DEVICE_ID 0x01
#define IR_TX_BUF_SIZE 32
#define STATION_TIMEOUT_TICKS 30000  // 3秒（sys_get_tick()返回0.1ms单位）

// ========== 全局变量 ==========
static ir_role_t current_role = IR_ROLE_STATION;
static station_state_t station_state = STATION_STATE_IDLE;
static robot_state_t robot_state = ROBOT_STATE_IDLE;
static uint8_t tx_buf[IR_TX_BUF_SIZE];

// 超时定时器
static uint64_t calibrate_timeout_start = 0;
static bool timeout_active = false;

// ========== 命令名称表（用于调试） ==========
const char* ir_cmd_name(uint8_t cmd)
{
    switch (cmd) {
    case IR_CMD_ROBOT_ARRIVE:      return "ARRIVE";
    case IR_CMD_STATION_CALIBRATE: return "CALIBRATE";
    case IR_CMD_ROBOT_CHARGE:      return "CHARGE";
    case IR_CMD_ROBOT_FULL:        return "FULL";
    case IR_CMD_STATION_RELOCATE:  return "RELOCATE";
    default:                       return "UNKNOWN";
    }
}

// ========== PWM载波控制（38kHz） ==========
void ir_pwm_init(void)
{
    if (current_role == IR_ROLE_STATION) {
        // PB6 - 使用TMRA_6 PWM8 (CMPAR8, PCONR8)
        __msl_printf("Init PWM: TMRA_6 PWM8 for PB6 (Station TX)\n");
        CM_PWC->FCG2 &= ~(1 << 11);  // 使能 TMRA_6 时钟

        CM_TMRA_6->BCSTRL = 0x02u;
        CM_TMRA_6->BCSTRH = 0x00u;
        CM_TMRA_6->CNTER = 0u;
        CM_TMRA_6->PERAR = 2632u;      // 100MHz/38kHz ≈ 2632
        CM_TMRA_6->CMPAR8 = 1316u;     // PWM8 50%占空比
        CM_TMRA_6->PCONR8 = 0x1243u;   // PWM8 初始关闭

        bCM_TMRA_6->BCSTRL_b.START = 1u;
    } else {
        // PD14 - 使用 TMR4_2 生成 PWM
        __msl_printf("Init PWM: TMR4_2 for PD14 (Robot TX test)\n");
        CM_PWC->FCG1 &= ~(1 << 3);   // 使能 TMR4_2 时钟

        // TMR4 PWM 配置
        CM_TMR4_2->OCCRUH = 2632u;    // 周期 (38kHz)
        CM_TMR4_2->OCCRUL = 1316u;    // 占空比 (50%)
        CM_TMR4_2->OCERU = 0x0001u;   // 使能 PWM 输出 U
        CM_TMR4_2->POCRU = 0x0010u;   // PWM 模式
        CM_TMR4_2->CCSR = 0x0001u;    // 启动计数器
    }
}

// 打开38kHz载波
void ir_carrier_on(void)
{
    if (current_role == IR_ROLE_STATION) {
        // PB6 - TMRA_6 PWM8 启用
        CM_TMRA_6->PCONR8 = 0x1043u;  // PWM8 输出启用
        // __msl_printf("Carrier ON (PB6)\n");
    } else {
        CM_TMR4_2->OCERU = 0x0001u;   // TMR4_2 PWM 启用
        // __msl_printf("Carrier ON (PD14)\n");
    }
}

// 关闭载波
void ir_carrier_off(void)
{
    if (current_role == IR_ROLE_STATION) {
        // PB6 - TMRA_6 PWM8 关闭
        CM_TMRA_6->PCONR8 = 0x1243u;  // PWM8 强制低电平
    } else {
        CM_TMR4_2->OCERU = 0x0000u;   // TMR4_2 PWM 关闭
    }
}

// ========== 数据编码发送 ==========
// 简化版编码（实际应用需要优化为微秒级）
void ir_send_bit(uint8_t bit)
{
    if (bit) {
        // '1': 载波1.5ms + 停止0.5ms
        ir_carrier_on();
        sys_delay_ms(1);
        ir_carrier_off();
        sys_delay_ms(1);
    } else {
        // '0': 载波0.5ms + 停止1.5ms
        ir_carrier_on();
        sys_delay_ms(1);
        ir_carrier_off();
        sys_delay_ms(1);
    }
}

void ir_send_byte(uint8_t data)
{
    __msl_printf("  TX byte: 0x%02x\n", data);
    for (int i = 7; i >= 0; i--) {
        ir_send_bit((data >> i) & 0x01);
    }
}

// 发送完整数据包
bool ir_send_packet(uint8_t cmd, uint8_t *data, uint8_t len)
{
    uint16_t crc;
    uint8_t idx = 0;
    const char* pin_name = (current_role == IR_ROLE_STATION) ? "PB6" : "PD14";
    
    __msl_printf("\n========== IR TX START ==========\n");
    __msl_printf("Role: %s\n", (current_role == IR_ROLE_STATION) ? "STATION" : "ROBOT");
    __msl_printf("TX Pin: %s\n", pin_name);
    __msl_printf("Command: %s (0x%02x)\n", ir_cmd_name(cmd), cmd);
    
    // 构造数据包
    tx_buf[idx++] = IR_PACKET_HEAD1;
    tx_buf[idx++] = IR_PACKET_HEAD2;
    tx_buf[idx++] = IR_DEVICE_ID;
    tx_buf[idx++] = cmd;
    tx_buf[idx++] = len;
    
    if (len > 0 && data != NULL) {
        for (uint8_t i = 0; i < len; i++) {
            tx_buf[idx++] = data[i];
        }
    }
    
    // 计算CRC（从ID到DATA）
    crc = calc_modbus_crc16(&tx_buf[2], len + 3);
    tx_buf[idx++] = DP_UINT16_H(crc);
    tx_buf[idx++] = DP_UINT16_L(crc);
    
    __msl_printf("CRC: 0x%04x\n", crc);
    __msl_printf("Total bytes: %d\n", idx);
    
    // 发送帧头（9ms载波 + 4.5ms停止）
    __msl_printf("Sending header pulse (9ms)...\n");
    ir_carrier_on();
    sys_delay_ms(9);
    ir_carrier_off();
    sys_delay_ms(4);
    
    // 发送数据字节
    __msl_printf("Sending data bytes:\n");
    for (uint8_t i = 0; i < idx; i++) {
        ir_send_byte(tx_buf[i]);
    }
    
    // 结束
    ir_carrier_off();
    
    __msl_printf("========== IR TX END ==========\n\n");
    return true;
}

// ========== 接收处理（简化版，轮询GPIO电平） ==========
void ir_receive_poll(void)
{
    static uint8_t last_level = 1;
    static uint32_t change_count = 0;
    uint8_t current_level;
    const char* pin_name;
    
    // 读取接收引脚电平
    if (current_role == IR_ROLE_STATION) {
        current_level = bCM_GPIO->PIDRB_b.PIN07;  // PB7 (原PB8已注释)
        pin_name = "PB7";
    } else {
        current_level = bCM_GPIO->PIDRD_b.PIN15;  // PD15
        pin_name = "PD15";
    }
    
    // 检测电平变化（用于调试）
    if (last_level != current_level) {
        change_count++;
        if (change_count % 50 == 0) {
            __msl_printf("[RX %s] Level changes: %d\n", pin_name, change_count);
        }
        last_level = current_level;
    }
    
    // TODO: 实现完整的解码逻辑
    // 实际应用需要使用定时器输入捕获测量脉宽
}

// 模拟接收（用于单板测试）
void ir_simulate_receive(uint8_t cmd)
{
    const char* pin_name = (current_role == IR_ROLE_STATION) ? "PB7" : "PD15";
    
    __msl_printf("\n========== IR RX (SIMULATED) ==========\n");
    __msl_printf("RX Pin: %s\n", pin_name);
    __msl_printf("Received: %s (0x%02x)\n", ir_cmd_name(cmd), cmd);
    ir_process_packet(cmd, NULL, 0);
    __msl_printf("=======================================\n\n");
}

// 处理接收到的数据包
void ir_process_packet(uint8_t cmd, uint8_t *data, uint8_t len)
{
    if (current_role == IR_ROLE_STATION) {
        // ===== 充电站侧逻辑 =====
        __msl_printf("[STATION] Current state: ");
        switch (station_state) {
        case STATION_STATE_IDLE:        __msl_printf("IDLE\n"); break;
        case STATION_STATE_CALIBRATING: __msl_printf("CALIBRATING\n"); break;
        case STATION_STATE_CHARGING:    __msl_printf("CHARGING\n"); break;
        }
        
        switch (station_state) {
        case STATION_STATE_IDLE:
            if (cmd == IR_CMD_ROBOT_ARRIVE) {
                __msl_printf("[STATION] Received ARRIVE -> Sending CALIBRATE\n");
                sys_delay_ms(100);  // 短暂延迟
                ir_send_calibrate();
                
                // 启动3秒超时定时器
                calibrate_timeout_start = sys_get_tick();
                timeout_active = true;
                station_state = STATION_STATE_CALIBRATING;
                __msl_printf("[STATION] 3-second timeout started\n");
            }
            break;
            
        case STATION_STATE_CALIBRATING:
            if (cmd == IR_CMD_ROBOT_CHARGE) {
                __msl_printf("[STATION] Received CHARGE -> Connecting relay\n");
                timeout_active = false;
                relay_connect();
                station_state = STATION_STATE_CHARGING;
            }
            break;
            
        case STATION_STATE_CHARGING:
            if (cmd == IR_CMD_ROBOT_FULL) {
                __msl_printf("[STATION] Received FULL -> Disconnecting relay\n");
                relay_disconnect();
                station_state = STATION_STATE_IDLE;
                __msl_printf("[STATION] Back to IDLE state\n");
            }
            break;
        }
        
    } else {
        // ===== 机器人侧逻辑 =====
        __msl_printf("[ROBOT] Current state: ");
        switch (robot_state) {
        case ROBOT_STATE_IDLE:     __msl_printf("IDLE\n"); break;
        case ROBOT_STATE_ARRIVED:  __msl_printf("ARRIVED\n"); break;
        case ROBOT_STATE_CHARGING: __msl_printf("CHARGING\n"); break;
        }
        
        switch (robot_state) {
        case ROBOT_STATE_ARRIVED:
            if (cmd == IR_CMD_STATION_CALIBRATE) {
                __msl_printf("[ROBOT] Received CALIBRATE -> Sending CHARGE\n");
                sys_delay_ms(100);
                ir_send_charge();
                robot_state = ROBOT_STATE_CHARGING;
            }
            break;
            
        case ROBOT_STATE_CHARGING:
            if (cmd == IR_CMD_STATION_RELOCATE) {
                __msl_printf("[ROBOT] Received RELOCATE -> Need reposition\n");
                robot_state = ROBOT_STATE_IDLE;
                __msl_printf("[ROBOT] Back to IDLE, waiting for repositioning\n");
            }
            break;
            
        default:
            break;
        }
    }
}

// ========== 超时检测 ==========
void ir_check_timeout(void)
{
    if (timeout_active && current_role == IR_ROLE_STATION) {
        uint64_t elapsed = sys_get_tick() - calibrate_timeout_start;
        
        if (elapsed >= STATION_TIMEOUT_TICKS) {
            __msl_printf("\n*** [STATION] TIMEOUT! ***\n");
            __msl_printf("[STATION] No CHARGE received within 3 seconds\n");
            __msl_printf("[STATION] Sending RELOCATE\n");
            timeout_active = false;
            ir_send_relocate();
            station_state = STATION_STATE_IDLE;
            __msl_printf("[STATION] Back to IDLE state\n\n");
        }
    }
}

// ========== 按钮检测（PE2） ==========
void ir_button_init(void)
{
    // PE2配置为输入，上拉（按钮按下接地）
    // 在io_init中已配置
    __msl_printf("Button init: PE2 (pull-up, active-low)\n");
}

bool ir_button_pressed(void)
{
    static uint8_t last_state = 1;
    static uint64_t debounce_time = 0;
    uint8_t current_state;
    
    current_state = bCM_GPIO->PIDRE_b.PIN02;  // 读取PE2
    
    // 检测下降沿（按下）
    if (last_state == 1 && current_state == 0) {
        // 防抖动：200ms
        if (sys_get_tick() - debounce_time > 2000) {
            debounce_time = sys_get_tick();
            last_state = current_state;
            __msl_printf("\n*** BUTTON PE2 PRESSED ***\n");
            return true;
        }
    }
    
    last_state = current_state;
    return false;
}

// ========== 公共接口实现 ==========
void ir_comm_init(ir_role_t role)
{
    current_role = role;
    
    __msl_printf("\n========================================\n");
    __msl_printf("       IR COMMUNICATION INIT\n");
    __msl_printf("========================================\n");
    __msl_printf("Role: %s\n", (role == IR_ROLE_STATION) ? "STATION" : "ROBOT");
    __msl_printf("TX Pin: %s\n", (role == IR_ROLE_STATION) ? "PB6" : "PD14");
    __msl_printf("RX Pin: %s\n", (role == IR_ROLE_STATION) ? "PB7" : "PD15");
    
    // 初始化PWM
    ir_pwm_init();
    
    // 初始化按钮
    ir_button_init();
    
    // 初始化继电器（仅充电站）
    if (role == IR_ROLE_STATION) {
        relay_ctrl_init();
    }
    
    __msl_printf("========================================\n\n");
}

bool ir_send_arrive(void)
{
    return ir_send_packet(IR_CMD_ROBOT_ARRIVE, NULL, 0);
}

bool ir_send_calibrate(void)
{
    return ir_send_packet(IR_CMD_STATION_CALIBRATE, NULL, 0);
}

bool ir_send_charge(void)
{
    return ir_send_packet(IR_CMD_ROBOT_CHARGE, NULL, 0);
}

bool ir_send_full(void)
{
    return ir_send_packet(IR_CMD_ROBOT_FULL, NULL, 0);
}

bool ir_send_relocate(void)
{
    return ir_send_packet(IR_CMD_STATION_RELOCATE, NULL, 0);
}

void ir_comm_poll(void)
{
    // 接收处理
    ir_receive_poll();
    
    // 超时检测
    ir_check_timeout();
    
    // 按钮检测（机器人模式）
    if (current_role == IR_ROLE_ROBOT && ir_button_pressed()) {
        __msl_printf("[TEST] Button triggered -> Sending ARRIVE\n");
        robot_state = ROBOT_STATE_ARRIVED;
        ir_send_arrive();
    }
}

// ========== 测试接口 ==========
void ir_test_send_cmd(uint8_t cmd)
{
    __msl_printf("\n[TEST] Manual send: %s\n", ir_cmd_name(cmd));
    ir_send_packet(cmd, NULL, 0);
}

void ir_print_status(void)
{
    __msl_printf("\n========== STATUS ==========\n");
    __msl_printf("Role: %s\n", (current_role == IR_ROLE_STATION) ? "STATION" : "ROBOT");
    
    if (current_role == IR_ROLE_STATION) {
        __msl_printf("State: ");
        switch (station_state) {
        case STATION_STATE_IDLE:        __msl_printf("IDLE\n"); break;
        case STATION_STATE_CALIBRATING: __msl_printf("CALIBRATING\n"); break;
        case STATION_STATE_CHARGING:    __msl_printf("CHARGING\n"); break;
        }
        if (timeout_active) {
            uint64_t elapsed = sys_get_tick() - calibrate_timeout_start;
            uint32_t remaining = (STATION_TIMEOUT_TICKS - elapsed) / 10;  // 转换为ms
            __msl_printf("Timeout: %d ms remaining\n", remaining);
        }
        __msl_printf("Relay: %s\n", (relay_get_state() == RELAY_STATE_ON) ? "ON" : "OFF");
    } else {
        __msl_printf("State: ");
        switch (robot_state) {
        case ROBOT_STATE_IDLE:     __msl_printf("IDLE\n"); break;
        case ROBOT_STATE_ARRIVED:  __msl_printf("ARRIVED\n"); break;
        case ROBOT_STATE_CHARGING: __msl_printf("CHARGING\n"); break;
        }
    }
    
    __msl_printf("===========================\n\n");
}