# Phase 2: NM Standardization Design

> 🟦 **Codex Agent** 输出 | 🟧 **Claude Code** 评审
>
> ⚠️ **历史快照**: 本文档记录 Phase 2 设计阶段的原始交付物。当前最新状态请参阅：
> - `NM_Comparison_README.md` — 项目管理 + 架构总览
> - `移植指南.md` — 集成使用指南
>
> Phase 3 实施中发生了以下架构变更（未回溯到本文档）：
> - 分发机制从 if/else 改为 **vtable 多态**（`CanNm_VtableType`）
> - 新增 `NM_MODE_AUTOSAR` + `CanNm_Autosar.c` 骨架
> - `CanNm_Direct/Indirect` 重命名为 `CanNm_Osek_Direct/Indirect`
> - 基础类型定义从 `Nm.h` 迁移至 `Nm_ConfigTypes.h`
>
> ---
>
> 基于 Phase 1 分析结果，定义统一 NM 模块的完整接口规范。

---

## 1. 交付物清单

| 文件 | 路径 | 大小 | 说明 |
|------|------|------|------|
| `Nm.h` | `NM\Nm.h` | 18KB | 统一 Nm_* 公开 API（19 个 API 函数） |
| `Nm_ConfigTypes.h` | `NM\Nm_ConfigTypes.h` | 8.6KB | 配置类型定义（含双线缆协议支持） |
| `Nm_Cbk.h` | `NM\Nm_Cbk.h` | 7.4KB | 回调函数声明（12 个回调） |
| `Nm_OsIf.h` | `NM\OsIf\Nm_OsIf.h` | 3.3KB | OS 抽象层（临界区 + 定时器） |
| `CanNm.h` | `NM\CanNm\CanNm.h` | 13KB | CanNm 内部接口（Nm Core ↔ CanNm） |

---

## 2. API 覆盖对比

| Nm.h API | 6AQ0 (Nm_*) | 6ER1 (OsekNm_*) | 标准化 |
|----------|:-----------:|:----------------:|:------:|
| `Nm_Init` | ✅ | ✅ (OsekNm_Init) | ✅ |
| `Nm_DeInit` | ❌ | ✅ (OsekNm_DeInit) | ✅ 新增，条件编译 |
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
| `Nm_ControllerBusOff` | ❌ | ✅ | ✅ 补齐 (6AQ0内部有OsekNm_BusOff) |
| `Nm_TxConfirmation` | ❌ (内部) | ✅ (内部) | ✅ 对外暴露统一接口 |
| `Nm_RxIndication` | ❌ (内部) | ✅ (内部) | ✅ 对外暴露统一接口 |

**汇总**: 19 个 API 覆盖两个项目的所有现有功能，补齐双方缺失项。

---

## 3. 核心设计决策

### 3.1 线缆协议双模支持

Phase 1 发现: 6AQ0 用 CAN ID 范围区分消息类型，6ER1 用 PDU OpCode bitfield。

**方案**: `Nm_WireFormatConfigType` 通过 `wireFormat` 字段选择模式：

```c
typedef struct {
    uint8  wireFormat;           /* NM_WIRE_FORMAT_CAN_ID 或 NM_WIRE_FORMAT_OPCODE */
    /* CAN_ID 模式 */
    uint16 canIdBase;            /* 0x400 */
    uint16 canIdMax;             /* 0x4FF */
    /* OpCode 模式 */
    uint16 singleCanId;          /* 所有 NM 消息用同一个 CAN ID */
} Nm_WireFormatConfigType;
```

两种模式的 PDU 格式定义分别在 `Nm_ConfigTypes.h` 中的 `NM_OPCODE_*` 和 `NM_OP_*_BIT` 宏中。

### 3.2 CFMOTO 自定义定时器包容

Phase 1 发现: 6ER1 有三个非标准定时器。

**方案**: `Nm_ChannelConfigType` 中有 `hasCfmotoExtensions` 标志位 + 三个扩展字段：
- `cfmotoTSleepRequestMin`
- `cfmotoTLimpSleepReqMin`
- `cfmotoTLimpHomeDTC`

仅在 `hasCfmotoExtensions == TRUE` 时有效，不影响标准 OSEK 行为。

### 3.3 配置方式统一

**方案**: 采用 6ER1 的结构体方式（优于 6AQ0 的全宏方式）：

```c
typedef struct {
    uint8                        numChannels;
    const Nm_ChannelConfigType*  channels;    /* 通道数组指针 */
    uint32                       mainFunctionPeriodMs;
} Nm_ConfigType;
```

项目在编译时定义 `static const Nm_ConfigType NmConfig = {...}` 并传入 `Nm_Init(&NmConfig)`。

### 3.4 回调机制

**方案**: 采用两层回调：
- **编译时回调**: `Nm_Cbk.h` 中声明的函数，应用层实现（6AQ0 风格，AUTOSAR 兼容）
- **运行时回调**: `Nm_ChannelConfigType` 中的函数指针 `stateChangeCallback` / `pduRxCallback`（6ER1 风格）

---

## 4. 目标目录结构

```
NM/                          <-- 标准化 NM 模块（可独立复用）
├── Nm.h                     ✅ 统一对外 API（19 个函数）
├── Nm_ConfigTypes.h         ✅ 配置结构体定义
├── Nm_Cbk.h                 ✅ 回调声明（12 个回调）
├── Nm_Internal.h            🔲 内部类型（待 Phase 3 实现时创建）
├── Nm.c                     🔲 NM 核心实现（路由、状态协调）
│
├── CanNm/
│   ├── CanNm.h              ✅ 内部接口（Nm Core ↔ CanNm）
│   ├── CanNm.c              🔲 适配层实现
│   ├── CanNm_Direct.c       🔲 Direct NM 状态机
│   ├── CanNm_Indirect.c     🔲 Indirect NM 状态机
│   └── CanNm_ConfigTypes.h  🔲 CanNm 特定配置
│
├── OsIf/
│   └── Nm_OsIf.h            ✅ OS 抽象层
│
└── test/
    └── test_nm_state.c      🔲 PC 端单元测试

图例: ✅ 已完成  🔲 待 Phase 3 实现
```

---

## 5. Phase 2 门禁自检

| # | 条件 | 状态 |
|---|------|:----:|
| 1 | Nm.h API 覆盖两个项目所有现有功能 + Phase 1 识别的缺失项 | ✅ |
| 2 | Nm_ConfigType 可以表达两项目的所有配置参数（含 CFMOTO 自定义定时器） | ✅ |
| 3 | 配置结构体能同时表达 CAN ID 模式和 OpCode 模式的线缆协议差异 | ✅ |
| 4 | Claude Code 确认设计方案在 6AQ0/6ER1 上均可落地 | 🔲 待评审 |

---

## 6. 下一步

> 🟧 **Claude Code**: 评审本设计文档 + NM 目录下的 5 个接口文件，确认设计在 6AQ0/6ER1 上均可落地。评审通过后 Phase 2 门禁全过，进入 Phase 3。

> 🟦 **Codex**: 等待评审意见，通过后更新 README 并规划 Phase 3 详细分工。

---

## 7. Claude Code 评审记录

**评审日期**: 2026-05-27  
**评审结论**: ⚠️ 有条件通过（3 个问题，1 个 blocker）

### 发现的问题及修复

| # | 问题 | 严重性 | 修复 |
|---|------|:------:|------|
| 1 | `NetworkHandleType` 在 `Nm.h` 和 `Nm_ConfigTypes.h` 重复定义 | 🔴 Blocker | ✅ 已修复：从 `Nm_ConfigTypes.h` 移除重复定义，保留在 `Nm.h` 单一定义 |
| 2 | `CANNM_STATE_ON` 在 `CanNm.h` 中无对应的 `NM_STATE_ON` | 🟡 建议 | ✅ 已修复：`Nm.h` 新增 `NM_STATE_ON = 0x0CU` |
| 3 | `canIdMsgAlive/canIdMsgRing/canIdMsgLimpHome` 命名暗示 CAN ID 偏移，实际是 PDU OpCode | 🟡 语义 | ✅ 已修复：重命名为 `pduOpCodeAlive/pduOpCodeRing/pduOpCodeLimpHome`，提升为两种线缆格式共用字段 |

### 最终门禁状态

| # | 条件 | 状态 |
|---|------|:----:|
| 1 | Nm.h API 覆盖全部功能 + 缺失项 | ✅ |
| 2 | ConfigType 表达全部配置参数 | ✅ |
| 3 | 双线缆协议支持 | ✅ |
| 4 | 两平台均可落地 | ✅ |

**Phase 2 门禁全部通过。可进入 Phase 3。**
