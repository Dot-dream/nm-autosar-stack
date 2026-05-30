# Phase2_Design 设计文档

> 属于 [[../00_MOC_总索引|MOC 总索引]] > **07_集成与附录**

基于 `NM/Phase2_Design.md` (2026-05-27) 的摘要版。完整设计文档含 Claude Code 评审记录，见源文件。

---

## 背景与目标

Phase 2 在 Phase 1（6AQ0 与 6ER1 差异分析）基础上，定义统一 NM 模块的完整接口规范。目标：

- 一个代码库覆盖 6AQ0 和 6ER1 两种产品的全部 NM 功能
- 通过配置切换 CAN ID 模式和 OpCode 模式的线缆协议
- 通过 vtable 多态支持三种 NM 类型（Direct / Indirect / AUTOSAR）

---

## 核心设计决策

### 1. vtable 多态

Phase 2 最初设计采用 `if/else` 分发。Phase 3 实施中改为 **`CanNm_VtableType` 函数指针表**：

```c
typedef struct CanNm_VtableType {
    void             (*Init)(const Nm_ChannelConfigType*, Nm_ChannelContextType*);
    void             (*DeInit)(NetworkHandleType);
    CanNm_ReturnType (*PassiveStartUp)(NetworkHandleType);
    CanNm_ReturnType (*NetworkRequest)(NetworkHandleType);
    CanNm_ReturnType (*NetworkRelease)(NetworkHandleType);
    void             (*DisableCommunication)(NetworkHandleType);
    void             (*EnableCommunication)(NetworkHandleType);
    void             (*SetUserData)(NetworkHandleType, const uint8*, uint8);
    void             (*GetUserData)(NetworkHandleType, uint8*, uint8*);
    void             (*GetPduData)(NetworkHandleType, uint8*);
    uint8            (*GetNodeIdentifier)(NetworkHandleType);
    uint8            (*GetLocalNodeIdentifier)(NetworkHandleType);
    boolean          (*CheckRemoteSleepIndication)(NetworkHandleType);
    void             (*GetState)(NetworkHandleType, Nm_StateType*, Nm_ModeType*);
    void             (*MainFunction)(NetworkHandleType);
    void             (*TxConfirmation)(NetworkHandleType);
    void             (*RxIndication)(NetworkHandleType, const uint8*, uint8);
    void             (*ControllerBusOff)(NetworkHandleType);
} CanNm_VtableType;
```

三种模式各自提供 18 个函数实现，`CanNm.c` 在初始化时按 `nmMode` 选择对应 vtable 实例并存入通道上下文。运行时通过 `ctx->vtable->xxx()` 单行调用，**消除所有分支判断**。

详见 [[../02_架构详解/vtable多态分发机制|vtable多态分发机制]]。

### 2. 双线缆协议

6AQ0 用 CAN ID 范围区分 NM 消息类型（Alive / Ring / LimpHome 各占一段 CAN ID），6ER1 用 PDU OpCode bitfield。

**方案**：`Nm_WireFormatConfigType` 通过 `wireFormat` 字段选择模式：

| 字段 | CAN ID 模式 | OpCode 模式 |
|------|------------|------------|
| `wireFormat` | `NM_WIRE_FORMAT_CAN_ID` | `NM_WIRE_FORMAT_OPCODE` |
| 关键配置 | `canIdBase` / `canIdMax` | `singleCanId` + 3 个 `pduOpCode*` |
| 占用 CAN ID | 多段范围 | 1 个 |
| 来源 | 6AQ0 | 6ER1 |

两种模式的 PDU 格式定义在 `Nm_ConfigTypes.h` 的 `NM_OPCODE_*` 和 `NM_OP_*_BIT` 宏中。

详见 [[../02_架构详解/双线缆协议设计|双线缆协议设计]]。

### 3. CFMOTO 扩展

6ER1 有三个非标准定时器。方案通过 `hasCfmotoExtensions` 标志位 + 三个扩展字段包容：

| 字段 | 说明 |
|------|------|
| `hasCfmotoExtensions` | `TRUE` 时三个扩展字段有效 |
| `cfmotoTSleepRequestMin` | 睡眠请求最小时间 |
| `cfmotoTLimpSleepReqMin` | LimpHome 睡眠请求最小时间 |
| `cfmotoTLimpHomeDTC` | LimpHome DTC 触发时间 |

仅在使能时影响行为，不影响标准 OSEK 状态机。

### 4. 配置方式统一

采用 6ER1 的结构体方式（优于 6AQ0 的全宏方式），项目定义 `const Nm_ConfigType NmConfig` 并传入 `Nm_Init(&NmConfig)`。

### 5. 回调机制

两层回调：
- **编译时回调**：`Nm_Cbk.h` 声明，应用层实现（AUTOSAR 兼容）
- **运行时回调**：`Nm_ChannelConfigType` 中的函数指针 `stateChangeCallback` / `pduRxCallback`

详见 [[../02_架构详解/回调通知机制|回调通知机制]]。

---

## API 覆盖对比表

| Nm.h API | 6AQ0 (Nm_*) | 6ER1 (OsekNm_*) | 标准化 |
|----------|:-----------:|:----------------:|:------:|
| `Nm_Init` | ✅ | ✅ (OsekNm_Init) | ✅ |
| `Nm_DeInit` | ❌ | ✅ (OsekNm_DeInit) | ✅ 新增, 条件编译 |
| `Nm_PassiveStartUp` | ✅ | ❌ | ✅ 补齐 |
| `Nm_NetworkRequest` | ✅ | ✅ | ✅ |
| `Nm_NetworkRelease` | ✅ | ✅ | ✅ |
| `Nm_DisableCommunication` | ✅ | ❌ | ✅ 补齐 |
| `Nm_EnableCommunication` | ✅ | ❌ | ✅ 补齐 |
| `Nm_SetUserData` | ✅ | ❌ | ✅ 补齐 |
| `Nm_GetUserData` | ✅ | ❌ | ✅ 补齐 |
| `Nm_GetPduData` | ✅ | ❌ | ✅ 补齐 |
| `Nm_RepeatMessageRequest` | ✅ | ❌ | ✅ 补齐 |
| `Nm_GetNodeIdentifier` | ✅ | ❌ | ✅ 补齐 |
| `Nm_GetLocalNodeIdentifier` | ✅ | ❌ | ✅ 补齐 |
| `Nm_CheckRemoteSleepIndication` | ✅ | ❌ | ✅ 补齐 |
| `Nm_GetState` | ✅ | ❌ (内部用) | ✅ |
| `Nm_GetVersionInfo` | ✅ (宏) | ✅ | ✅ 统一为函数 |
| `Nm_MainFunction` | ✅ | ✅ | ✅ |
| `Nm_ControllerBusOff` | ❌ | ✅ | ✅ 补齐 |
| `Nm_TxConfirmation` | ❌ (内部) | ✅ (内部) | ✅ 对外暴露统一接口 |
| `Nm_RxIndication` | ❌ (内部) | ✅ (内部) | ✅ 对外暴露统一接口 |

**19 个 API**，覆盖两个项目的所有现有功能，补齐双方缺失项。

---

## 目标目录结构（Phase 2 设计时）

```
NM/
├── Nm.h                     ✅ 统一对外 API（19 个函数）
├── Nm_ConfigTypes.h         ✅ 配置结构体定义
├── Nm_Cbk.h                 ✅ 回调声明（12 个回调）
├── Nm_Internal.h            🔲 内部类型（Phase 3 实现）
├── Nm.c                     🔲 NM 核心实现
├── CanNm/
│   ├── CanNm.h              ✅ 内部接口
│   ├── CanNm.c              🔲 适配层
│   ├── CanNm_Direct.c       🔲 Direct NM（后重命名为 CanNm_Osek_Direct.c）
│   ├── CanNm_Indirect.c     🔲 Indirect NM（后重命名为 CanNm_Osek_Indirect.c）
│   └── CanNm_ConfigTypes.h  🔲 CanNm 特定配置
├── OsIf/
│   └── Nm_OsIf.h            ✅ OS 抽象层
└── test/
    └── test_nm_state.c      🔲 PC 端测试

图例: ✅ 已完成  🔲 待 Phase 3 实现
```

---

## Phase 3 实施中的架构变更

Phase 3 实施中发生了以下未回溯到 Phase2_Design.md 的变更：

| 变更 | 说明 |
|------|------|
| 分发机制从 `if/else` 改为 **vtable 多态** | `CanNm_VtableType` 替代分支判断 |
| 新增 `NM_MODE_AUTOSAR` + `CanNm_Autosar.c` 骨架 | 预留 AUTOSAR NM 支持 |
| `CanNm_Direct/Indirect` 重命名为 `CanNm_Osek_Direct/Indirect` | 语义明确（OSEK 协议） |
| 基础类型定义从 `Nm.h` 迁移至 `Nm_ConfigTypes.h` | 消除重复定义，单一数据源 |
| `CanNm_ConfigTypes.h` 合并入 `Nm_ConfigTypes.h` | 简化头文件结构 |

---

## Claude Code 评审记录

**评审日期**: 2026-05-27 | **结论**: ⚠️ 有条件通过（3 个问题修复后全部通过）

| # | 问题 | 严重性 | 修复 |
|:--:|------|:------:|------|
| 1 | `NetworkHandleType` 在 `Nm.h` 和 `Nm_ConfigTypes.h` 重复定义 | 🔴 Blocker | ✅ 从 `Nm_ConfigTypes.h` 移除 |
| 2 | `CANNM_STATE_ON` 无对应的 `NM_STATE_ON` | 🟡 建议 | ✅ `Nm.h` 新增 `NM_STATE_ON = 0x0CU` |
| 3 | `canIdMsgAlive/canIdMsgRing/canIdMsgLimpHome` 命名暗示 CAN ID 偏移 | 🟡 语义 | ✅ 重命名为 `pduOpCode*`，提升为两种线缆格式共用字段 |

**Phase 2 门禁全部通过**，进入 Phase 3 实施。

---

> 完整设计文档见 `NM/Phase2_Design.md`。移植指南见 [[移植指南|移植指南]]。原始项目对照见 [[原始项目对照表|原始项目对照表]]。
