# NM 标准化模块简介

> 属于 [[../00_MOC_总索引|MOC 总索引]] > **01_概述**

---

## 1. 项目背景

NM（Network Management，网络管理）是 AUTOSAR BSW（基础软件）中的核心通信服务模块，
负责协调 CAN 总线上多个 ECU 节点的休眠与唤醒。

在标准化之前，团队的两个项目（6AQ0 和 6ER1）使用了两套不同的 NM 实现：
- **6AQ0**: iSOFT `NmIf` + `OsekNm`，基于 CAN ID 范围的消息格式
- **6ER1**: Arctic Core `OsekNm`，基于 PDU OpCode bitfield 的消息格式

标准化模块统一了这两套实现，提供了一套**可复用、可配置、可测试**的 NM 组件。

参考规范:
- AUTOSAR Nm 4.2.2
- OSEK NM 2.5.3

---

## 2. 核心目标

| 目标 | 说明 |
|------|------|
| **统一接口** | 19 个 `Nm_*()` API 覆盖 Direct / Indirect / AUTOSAR 三种 NM 模式 |
| **配置驱动** | 所有参数通过 `Nm_ConfigType` 结构体传入，无 `#define` 硬编码 |
| **vtable 多态** | `CanNm_VtableType` 3×18 函数指针表，新增 NM 类型无需修改适配层 |
| **零平台依赖** | 核心逻辑不依赖任何 MCU / RTOS，仅需 3 个平台函数 |
| **PC 可测试** | `-DNM_HOST_TEST` shim 模式，7 个测试用例在 PC 上验证状态机 |
| **可裁剪** | 15 个编译开关，未启用功能的代码在编译期完全剔除 |

---

## 3. 设计原则

1. **分层解耦**: App → Nm Core → CanNm 适配层 → 状态机 → 平台抽象，每层职责单一
2. **编译时多态**: 通过 vtable 静态分配 + 编译开关实现零运行时开销的多态
3. **状态显式管理**: 所有状态转换通过 `ChangeState()` 函数，回调通知在转换时同步触发
4. **定时器外置**: NM 核心不知道定时器实现细节，仅通过 `Nm_Timer` 中间件接口交互
5. **双线缆协议兼容**: 同时支持 CAN ID 模式 (`0x400-0x4FF`) 和 OpCode 模式 (单一 CAN ID)

---

## 4. 适用场景

| 场景 | NM 模式 | 说明 |
|------|---------|------|
| 动力 CAN (PT-CAN) | OSEK Direct | 严格的 Alive/Ring 逻辑环，多节点协调休眠 |
| 车身 CAN (Body-CAN) | OSEK Indirect | 不发送 NM PDU，通过监控应用消息判定活跃 |
| AUTOSAR 标准平台 | AUTOSAR NM | CBV 广播协调，支持 Partial Networking |
| 单通道多协议 | 混合 | 不同 CAN 通道配置不同 nmMode |

---

## 5. 关键数字

| 指标 | 数值 |
|------|:--:|
| 公开 API 函数 | 19 个 |
| 回调函数 | 12 个 |
| NM 模式 | 3 种 (Direct / Indirect / AUTOSAR) |
| OSEK Direct 状态 | 10 个 (OFF→INIT→INITRESET→NORMAL→NORMALPREPSLEEP→TWBS_NORMAL→BUSSLEEP→LIMPHOME→LIMPHOMEPREPSLEEP→TWBS_LIMPHOME) |
| OSEK Indirect 状态 | 7 个 (OFF→INIT→AWAKE→NORMAL→WAITBUSSLEEP→BUSSLEEP→LIMPHOME) |
| AUTOSAR NM 状态 | 7 个 (UNINIT→BUS_SLEEP→REPEAT_MESSAGE→NORMAL_OPERATION→READY_SLEEP→PREPARE_BUS_SLEEP→SYNCHRONIZE) |
| vtable 函数指针/模式 | 18 个 |
| 最大通道数 | 8 |
| PDU 最大长度 | 8 字节 |
| 定时器池容量 | 40 个 |
| 编译开关 | 15 个 |
| 平台适配函数 | 3 个 (Tick + 临界区 × 2 + CAN 收发 × 3) |
| PC 测试用例 | 7 个 |
| 源文件总数 | 14 个 |
| 总代码行数 | ~3700 行 |

---

## 6. 与原始项目的差异

| 维度 | 6AQ0 (原始) | 6ER1 (原始) | NM 标准化模块 |
|------|:----------:|:----------:|:------------:|
| 代码量 | ~200KB | ~120KB | ~120KB |
| API 层 | `NmIf` + `Nm` 双层 | 仅 `OsekNm` | 统一 `Nm_*` |
| NM 模式 | Direct only | Direct + Indirect | Direct + Indirect + AUTOSAR |
| 分发方式 | `if/else` 分支 | 同一文件内 switch | vtable 多态 |
| 定时器 | 内嵌 decrement | 内嵌 decrement | 独立 Nm_Timer 中间件 |
| 线缆协议 | CAN ID 模式 | OpCode 模式 | 双模支持 |
| PC 测试 | 不支持 | 不支持 | 7 个测试用例 |
| 代码风格 | `typedef unsigned char uint8` | `typedef unsigned char uint8` | 统一 `uint8/uint16/uint32` |

---

> 下一步: 阅读 [[../01_概述/三种NM模式对比|三种 NM 模式对比]]
