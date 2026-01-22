#ifndef __IR_COMM_H_
#define __IR_COMM_H_

#include "system_hc32f460.h"
#include <stdbool.h>

// ========== 协议命令定义 ==========
enum {
    IR_CMD_IDLE = 0x00,
    IR_CMD_ROBOT_ARRIVE = 0x01,      // 机器人→充电站："到位"
    IR_CMD_STATION_CALIBRATE = 0x82, // 充电站→机器人："校准"（响应标志）
    IR_CMD_ROBOT_CHARGE = 0x03,      // 机器人→充电站："充电"
    IR_CMD_ROBOT_FULL = 0x04,        // 机器人→充电站："已充满"
    IR_CMD_STATION_RELOCATE = 0x84,  // 充电站→机器人："重定位"（响应标志）
};

// ========== 角色定义 ==========
typedef enum {
    IR_ROLE_STATION = 0,  // 充电站模式（PB8接收/PB9发射）
    IR_ROLE_ROBOT = 1     // 机器人测试模式（PD15接收/PD14发射）
} ir_role_t;

// ========== 状态机定义 ==========
typedef enum {
    STATION_STATE_IDLE = 0,      // 空闲等待
    STATION_STATE_CALIBRATING,   // 已发送校准，等待充电命令（3秒超时）
    STATION_STATE_CHARGING,      // 充电中
} station_state_t;

typedef enum {
    ROBOT_STATE_IDLE = 0,
    ROBOT_STATE_ARRIVED,    // 已到位，等待校准
    ROBOT_STATE_CHARGING,   // 充电中
} robot_state_t;

// ========== 初始化 ==========
void ir_comm_init(ir_role_t role);

// ========== 发送接口 ==========
bool ir_send_arrive(void);      // 机器人："到位"
bool ir_send_calibrate(void);   // 充电站："校准"
bool ir_send_charge(void);      // 机器人："充电"
bool ir_send_full(void);        // 机器人："已充满"
bool ir_send_relocate(void);    // 充电站："重定位"

// ========== 主循环 ==========
void ir_comm_poll(void);

// ========== 测试接口 ==========
void ir_test_send_cmd(uint8_t cmd);  // 手动发送指定命令
void ir_print_status(void);          // 打印当前状态
void ir_simulate_receive(uint8_t cmd); // 模拟接收命令（测试用）

// ========== 按钮检测 ==========
void ir_button_init(void);           // 初始化PE2按钮
bool ir_button_pressed(void);        // 检测按钮是否按下

#endif