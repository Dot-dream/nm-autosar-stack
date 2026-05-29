# 项目框架架构

## 目录结构

```
Project_Root/
Project_Root/
├── 📁 App/                    # 应用层 - 所有业务逻辑
│   ├── 📁 Modules/           # 功能模块（按功能划分）
│   │   ├── module_can.c/.h   # CAN通信模块
│   │   ├── module_control.c/.h # 控制模块
│   │   └── module_power.c/.h # 电源管理模块
│   ├── 📁 System/            # 系统核心功能
│   │   ├── scheduler.c/.h    # 任务调度器
│   │   ├── debug.c/.h        # 调试输出系统
│   │   └── utils.c/.h        # 系统工具函数
│   ├── 📁 Config/            # 应用配置
│   │   ├── app_config.h      # 应用通用配置
│   │   ├── vehicle_config.h  # 车辆配置（车型相关）
│   │   └── task_config.h     # 任务系统配置
│   └── main.c               # 主程序入口
│
├── 📁 Bsp/                   # 板级支持包 - 硬件抽象层
│   ├── 📁 Hal/               # 硬件抽象层 (HAL) - 标准API接口
│   │   ├── 📄 hal_gpio.h/.c
│   │   ├── 📄 hal_can.h/.c
│   │   ├── 📄 hal_uart.h/.c
│   │   ├── 📄 hal_adc.h/.c
│   │   ├── 📄 hal_timer.h/.c
│   │   ├── 📄 hal_pwm.h/.c
│   │   ├── 📄 hal_flash.h/.c
│   │   └── 📄 hal_system.h/.c
│   │
│   └── 📁 Middleware/        # 中间件层 - 平台适配实现
│       ├── 📄 mw_gpio.h/.c
│       ├── 📄 mw_can.h/.c
│       ├── 📄 mw_uart.h/.c
│       ├── 📄 mw_adc.h/.c
│       ├── 📄 mw_timer.h/.c
│       ├── 📄 mw_pwm.h/.c
│       ├── 📄 mw_flash.h/.c
│       ├── 📄 mw_system.h/.c
│       ├── 📄 platform_port.h      # 平台移植端口文件
│       ├── 📄 mw_config.h          # 中间件配置
│       └── 📄 mw_porting_guide.md  # 移植指南
│
├── 📁 Platform/              # 平台层 - 硬件驱动/SDK
│   ├── 📁 Current_Platform/  # 当前使用的平台（如Yuntu）
│   │   ├── 📁 SDK/           # 厂商SDK
│   │   ├── 📁 Drivers/       # 平台特有驱动
│   │   ├── 📁 Startup/       # 启动文件
│   │   ├── 📁 Linker/        # 链接脚本
│   │   └── platform_info.md  # 平台信息说明
│   │
│   └── 📁 Platform_Templates/# 其他平台模板（用于新平台移植参考）
│       ├── STM32_Template/
│       └── GD32_Template/
│
├── 📁 Lib/                   # 第三方库/通用库
│   ├── 📁 Protocol/          # 通信协议库
│   │   └── simple_can/       # 简易CAN协议处理
│   │
│   ├── 📁 Algorithm/         # 实用算法库
│   │   ├── pid_controller.c/.h
│   │   ├── moving_average.c/.h
│   │   └── crc_check.c/.h
│   │
│   ├── 📁 Utility/           # 实用工具库
│   │   ├── ring_buffer.c/.h
│   │   └── bit_operations.c/.h
│   │
│   └── 📁 Safety/            # 安全相关库（针对车身控制）
│       └── watchdog_client.c/.h
│
├── 📁 Build/                 # 构建输出目录
│   ├── debug/               # 调试版本
│   ├── release/             # 发布版本
│   └── production/          # 生产版本
│
├── 📁 Tools/                 # 开发工具和脚本
│   ├── 📁 Scripts/          # 构建和调试脚本
│   │   ├── build_project.py
│   │   ├── flash_firmware.py
│   │   └── generate_config.py
│   │
│   └── 📁 Utilities/         # 实用工具
│       └── can_monitor.py    # CAN总线监视工具
│
├── 📄 README.md             # 项目总说明
├── 📄 project_config.h      # 项目全局配置
├── 📄 coding_standard.h     # 编码规范内联文档
├── 📄 .gitignore           # Git忽略配置
└── 📄 license.txt         # 许可证文件

## 功能描述

- **App**：包含主程序文件 `main.c`，调用硬件抽象层 (HAL) 的接口。
- **Bsp**：
  - **Hal**：硬件抽象层，提供标准化的硬件接口。
  - **Middleware**：适配层，针对具体平台实现 HAL 接口。
- **Drivers**：厂商提供的驱动库。