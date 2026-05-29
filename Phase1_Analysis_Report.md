# Phase 1: NM Module Deep-Dive Analysis Report

> 🟩 **Claude Code Agent** 输出 | 🟦 Codex Agent 评审
>
> 本文档对应 `NM_Comparison_README.md` 中 Phase 1 的 1.1~1.5 任务。
> 所有结论基于源码阅读，不依赖推测。

---

## 1.1 API 级别对比

### 1.1.1 6AQ0 Nm 层公开 API（Nm.h）

| # | 函数 | 签名 | 条件编译 | 返回类型 |
|---|------|------|----------|----------|
| 1 | `Nm_Init` | `void Nm_Init(void)` | 无条件 | void |
| 2 | `Nm_PassiveStartUp` | `Nm_ReturnType Nm_PassiveStartUp(NetworkHandleType)` | 无条件 | Nm_ReturnType |
| 3 | `Nm_NetworkRequest` | `Nm_ReturnType Nm_NetworkRequest(NetworkHandleType)` | `NM_PASSIVE_MODE_ENABLED == OFF` | Nm_ReturnType |
| 4 | `Nm_NetworkRelease` | `Nm_ReturnType Nm_NetworkRelease(NetworkHandleType)` | `NM_PASSIVE_MODE_ENABLED == OFF` | Nm_ReturnType |
| 5 | `Nm_DisableCommunication` | `Nm_ReturnType Nm_DisableCommunication(NetworkHandleType)` | `NM_COM_CONTROL_ENABLED == ON` | Nm_ReturnType |
| 6 | `Nm_EnableCommunication` | `Nm_ReturnType Nm_EnableCommunication(NetworkHandleType)` | `NM_COM_CONTROL_ENABLED == ON` | Nm_ReturnType |
| 7 | `Nm_SetUserData` | `Nm_ReturnType Nm_SetUserData(NetworkHandleType, const uint8*)` | `NM_USER_DATA_ENABLED == ON` | Nm_ReturnType |
| 8 | `Nm_GetUserData` | `Nm_ReturnType Nm_GetUserData(NetworkHandleType, uint8*, uint8*)` | `NM_USER_DATA_ENABLED == ON` | Nm_ReturnType |
| 9 | `Nm_GetPduData` | `Nm_ReturnType Nm_GetPduData(NetworkHandleType, uint8*)` | NodeID/Detection/UserData 任一启用 | Nm_ReturnType |
| 10 | `Nm_RepeatMessageRequest` | `Nm_ReturnType Nm_RepeatMessageRequest(NetworkHandleType)` | `NM_NODE_DETECTION_ENABLED == ON` | Nm_ReturnType |
| 11 | `Nm_GetNodeIdentifier` | `Nm_ReturnType Nm_GetNodeIdentifier(NetworkHandleType, uint8*)` | `NM_NODE_ID_ENABLED == ON` | Nm_ReturnType |
| 12 | `Nm_GetLocalNodeIdentifier` | `Nm_ReturnType Nm_GetLocalNodeIdentifier(NetworkHandleType, uint8*)` | `NM_NODE_ID_ENABLED == ON` | Nm_ReturnType |
| 13 | `Nm_CheckRemoteSleepIndication` | `Nm_ReturnType Nm_CheckRemoteSleepIndication(NetworkHandleType, BOOLEAN*)` | `NM_REMOTE_SLEEP_IND_ENABLE == ON` | Nm_ReturnType |
| 14 | `Nm_GetState` | `Nm_ReturnType Nm_GetState(NetworkHandleType, Nm_StateType*, Nm_ModeType*)` | 无条件 | Nm_ReturnType |
| 15 | `Nm_GetVersionInfo` | 宏展开为 `Std_VersionInfoType*` 赋值 | `NM_VERSION_INFO_API == ON` | void (宏) |
| 16 | `Nm_MainFunction` | `void Nm_MainFunction(void)` | `NM_COORDINATOR_SUPPORT_ENABLED == ON` | void |

**返回类型**: `Nm_ReturnType` 枚举值为 `NM_E_OK` / `NM_E_NOT_OK` / `NM_E_NOT_EXECUTED`。

### 1.1.2 6AQ0 Nm 层回调 API（Nm_Cbk.h）

这些是 Nm 层向上（ComM）通知的回调函数：

| # | 回调 | 用途 | 条件编译 |
|---|------|------|----------|
| C1 | `Nm_NetworkStartIndication(NetworkHandleType)` | 在 Bus-Sleep 模式收到 NM 消息（唤醒） | 无条件 |
| C2 | `Nm_NetworkMode(NetworkHandleType)` | NM 进入 Network Mode | 无条件 |
| C3 | `Nm_PrepareBusSleepMode(NetworkHandleType)` | NM 进入 Prepare Bus-Sleep | 无条件 |
| C4 | `Nm_BusSleepMode(NetworkHandleType)` | NM 进入 Bus-Sleep | 无条件 |
| C5 | `Nm_PduRxIndication(NetworkHandleType)` | NM PDU 已接收 | `NM_PDU_RX_INDICATION_ENABLED` |
| C6 | `Nm_RemoteSleepIndication(NetworkHandleType)` | 远程睡眠指示（协调器） | `NM_COORDINATOR_SUPPORT_ENABLED` |
| C7 | `Nm_RemoteSleepCancelation(NetworkHandleType)` | 远程睡眠取消（协调器） | `NM_COORDINATOR_SUPPORT_ENABLED` |
| C8 | `Nm_StateChangeNotification(NetworkHandleType, Nm_StateType, Nm_StateType)` | 状态变更（含前/后状态） | `NM_STATE_CHANGE_IND_ENABLED` |
| C9 | `Nm_TxTimeoutException(NetworkHandleType)` | 发送超时 | `NM_PASSIVE_MODE_ENABLED == OFF` |

### 1.1.3 6AQ0 OsekNm 层内部 API（OsekNm.h, OsekNm.c）

OsekNm 层是 Nm 层的下层，由 Nm 层调用。对外（应用层）不可见。

| # | 函数 | 行号 | 说明 |
|---|------|------|------|
| 1 | `OsekNm_PassiveStartUp` | 579 | Bus-Sleep → 设置 REQUEST 事件 |
| 2 | `OsekNm_NetworkRequest` | 618 | 清除 NETWORKRLEASED 标志 → 设置 REQUEST 事件 |
| 3 | `OsekNm_NetworkRelease` | 657 | 设置 NETWORKRLEASED 标志 |
| 4 | `OsekNm_DisableCommunication` | 706 | 禁用 NM PDU 发送 |
| 5 | `OsekNm_EnableCommunication` | 748 | 启用 NM PDU 发送 |
| 6 | `OsekNm_SetUserData` | 790 | 设置用户数据到 NM PDU |
| 7 | `OsekNm_GetUserData` | 835 | 获取最近收到的用户数据 |
| 8 | `OsekNm_GetNodeIdentifier` | 883 | 获取收到的源节点 ID |
| 9 | `OsekNm_GetLocalNodeIdentifier` | 915 | 获取本地节点 ID |
| 10 | `OsekNm_RepeatMessageRequest` | 945 | 设置 Repeat Message Request |
| 11 | `OsekNm_GetPduData` | 972 | 获取整个 PDU 数据 |
| 12 | `OsekNm_GetState` | 1008 | 返回状态+模式 |
| 13 | `OsekNm_CheckRemoteSleepIndication` | 1069 | 检查远程睡眠指示 |
| 14 | `OsekNm_BusOff` | 1302 | 总线关闭处理 |
| 15 | `OsekNm_BusOffRecovery` | 1343 | 总线恢复处理 |
| 16 | `OsekNm_MainFunction` | 1390 | 主循环：递减定时器、处理事件 |
| 17 | `OsekNm_LimpHomeDTCReset` | 1385 | 重置 LimpHome DTC |
| 18 | `OsekNm_SendMessage` | 2570 | 发送 NM 消息 |

### 1.1.4 6ER1 公开 API（OsekNm.h）

| # | 函数 | 签名 | 返回类型 |
|---|------|------|----------|
| 1 | `OsekNm_Init` | `void OsekNm_Init(void)` | void |
| 2 | `OsekNm_DeInit` | `void OsekNm_DeInit(NetIdType netId)` | void |
| 3 | `OsekNm_NetworkRequest` | `Std_ReturnType OsekNm_NetworkRequest(NetworkHandleType)` | Std_ReturnType |
| 4 | `OsekNm_NetworkRelease` | `Std_ReturnType OsekNm_NetworkRelease(NetworkHandleType)` | Std_ReturnType |
| 5 | `OsekNm_TxConfirmation` | `void OsekNm_TxConfirmation(NetIdType, NodeIdType)` | void |
| 6 | `OsekNm_RxIndication` | `void OsekNm_RxIndication(NetIdType, NodeIdType, const OsekNm_PduType*)` | void |
| 7 | `OsekNm_MainFunction` | `void OsekNm_MainFuntion(void)` | void |
| 8 | `OsekNm_ControllerBusOff` | `void OsekNm_ControllerBusOff(NetIdType)` | void |
| 9 | `OsekNm_TimeoutNotification` | `void OsekNm_TimeoutNotification(NetIdType, NodeIdType)` | void |
| 10 | `OsekNm_GetVersionInfo` | `void OsekNm_GetVersionInfo(Std_VersionInfoType*)` | void |
| 11 | `OsekNm_InitCMaskTable` | `void OsekNm_InitCMaskTable(NetIdType, ConfigKindName, ConfigRefType)` | void |
| 12 | `OsekNm_InitTargetConfigTable` | `void OsekNm_InitTargetConfigTable(NetIdType, ConfigKindName, ConfigRefType)` | void |
| 13 | `OsekNm_CmpConfig` | `StatusType OsekNm_CmpConfig(NetIdType, ConfigRefType, ConfigRefType, ConfigRefType)` | StatusType |
| 14 | `OsekNm_GetConfig` | `StatusType OsekNm_GetConfig(NetIdType, ConfigRefType, ConfigKindName)` | StatusType |
| 15 | `OsekNm_GetStatus` | `StatusType OsekNm_GetStatus(NetIdType, StatusRefType)` | StatusType |

此外，6ER1 OsekNm.h 还暴露了 DLL 层接口（D_Init, D_Offline, D_Online, CanNM_TransmitACK, D_Window_Data_req）和总线控制接口（BusInit, BusSleep, BusAwake, BusRestart, BusShutdown, CanNm_CommCtl, CanNm_RxEnabled, CanNm_TxEnabled），以及应用钩子（CanWakeup_DoAction, AccOn_DoAction, AccOff_DoAction）。

### 1.1.5 API 映射表：6AQ0 Nm_* ↔ 6ER1 OsekNm_*

| 6AQ0 (Nm 层) | 6ER1 (OsekNm) | 映射关系 |
|---------------|---------------|----------|
| `Nm_Init` | `OsekNm_Init` | **近似** — 6ER1 额外有 DeInit，6AQ0 无 |
| `Nm_PassiveStartUp` | (无) | **缺失** — 6ER1 无独立 PassiveStartUp，功能集成在 GotoMode |
| `Nm_NetworkRequest` | `OsekNm_NetworkRequest` | **近似** — 签名相同，内部实现机制不同 |
| `Nm_NetworkRelease` | `OsekNm_NetworkRelease` | **近似** — 签名相同 |
| `Nm_DisableCommunication` | `CanNm_CommCtl(NM_TX, 0)` | **近似** — 6AQ0 有独立 API，6ER1 用内部函数 |
| `Nm_EnableCommunication` | `CanNm_CommCtl(NM_TX, 1)` | **近似** — 同上 |
| `Nm_SetUserData` | (无) | **缺失** — 6ER1 无独立用户数据 API（通过 PDU ring data） |
| `Nm_GetUserData` | (无) | **缺失** — 同上 |
| `Nm_GetPduData` | (无) | **缺失** — 6ER1 通过 RxIndication 回调获取 PDU |
| `Nm_RepeatMessageRequest` | (无) | **缺失** — 6ER1 无此功能 |
| `Nm_GetNodeIdentifier` | (无) | **缺失** — 6ER1 通过 PDU source 字段获取 |
| `Nm_GetLocalNodeIdentifier` | (无) | **缺失** — 6ER1 通过配置结构体获取 |
| `Nm_CheckRemoteSleepIndication` | (无) | **缺失** — 6ER1 通过状态机内部 SleepInd 处理 |
| `Nm_GetState` | `OsekNm_GetStatus` | **近似** — 6AQ0 返回 State+Mode 枚举，6ER1 返回 bitfield |
| `Nm_GetVersionInfo` | `OsekNm_GetVersionInfo` | **匹配** — 语义相同 |
| `Nm_MainFunction` | `OsekNm_MainFunction` | **近似** — 6AQ0 只在协调器模式下编译 |
| (无) | `OsekNm_DeInit` | **多余** — 6AQ0 无 DeInit |
| (无) | `OsekNm_TxConfirmation` | **多余** — 6AQ0 使用立即 TxConf 模式 |
| (无) | `OsekNm_RxIndication` | **多余** — 6AQ0 通过 PduR 直接路由 |
| (无) | `OsekNm_ControllerBusOff` | **多余** — 6AQ0 BusOff 在内部处理 |
| (无) | `OsekNm_InitCMaskTable` 等 4 个 | **多余** — 6AQ0 无配置验证 API |

---

## 1.2 状态机对比

### 1.2.1 6AQ0 — 两层状态机

#### 上层：Nm 层状态机 (NmStack_Types.h)

```
                    ┌─────────┐
                    │ UNINIT  │
                    └────┬────┘
                         │ Nm_Init()
                         ▼
                    ┌─────────┐
          ┌────────│BUS_SLEEP│◄──────────────┐
          │        └────┬────┘                │
          │             │ NetworkStartInd     │
          │             ▼                     │
          │    ┌───────────────┐              │
          │    │REPEAT_MESSAGE │              │
          │    └───────┬───────┘              │
          │            │ 消息交换完成          │
          │            ▼                     │
          │    ┌────────────────┐            │
          │    │NORMAL_OPERATION│            │
          │    └───────┬────────┘            │
          │            │ NetworkRelease       │
          │            ▼                     │
          │    ┌──────────────────┐          │
          │    │PREPARE_BUS_SLEEP │          │
          │    └────────┬─────────┘          │
          │             │ TWbs 超时           │
          │             ▼                     │
          │    ┌──────────────┐              │
          │    │ READY_SLEEP  │──────────────┘
          │    └──────────────┘
          │
          └─── SYNCHRONIZE (总线同步，当前关闭)
```

**Nm_ModeType** (网络模式):
- `NM_MODE_BUS_SLEEP` (0)
- `NM_MODE_PREPARE_BUS_SLEEP` (1)
- `NM_MODE_SYNCHRONIZE` (2)
- `NM_MODE_NETWORK` (3)

#### 下层：OsekNm 层状态机 (OsekNm.c 内部)

OsekNm 层的状态通过 `OsekNm_ChannelType.status` bitfield 编码：

| 状态位 | 宏 | 含义 |
|--------|-----|------|
| bit 2 | `OSEKNM_STATUS_ACTIVE` (0x0004) | 网络活跃中 |
| bit 3 | `OSEKNM_STATUS_OFF` (0x0008) | NM 关闭 |
| bit 4 | `OSEKNM_STATUS_LIMPHOME` (0x0010) | LimpHome 状态 |
| bit 5 | `OSEKNM_STATUS_BUSSLEEP` (0x0020) | Bus-Sleep 状态 |
| bit 6 | `OSEKNM_STATUS_TWBS` (0x0040) | 等待 Bus-Sleep |
| bit 7 | `OSEKNM_STATUS_RINGDATA_NOTALLOWED` (0x0080) | 禁止 ring data |
| bit 8 | `OSEKNM_STATUS_NETWORKRLEASED` (0x0100) | 网络已释放 |
| bit 9 | `OSEKNM_STATUS_MERKER_STABLE` (0x0200) | 配置稳定标记 |
| bit 10 | `OSEKNM_STATUS_MERKER_LIMPHOME` (0x0400) | LimpHome 标记 |
| bit 11 | `OSEKNM_STATUS_REMOTE_SLEEPIND` (0x0800) | 远程睡眠指示 |

内部事件 (event bitfield):
- `OSEKNM_EVENT_REQUEST` (0x01) — 网络请求/释放
- `OSEKNM_EVENT_TXFLAG` (0x02) — 发送标志
- `OSEKNM_EVENT_RXIND` (0x04) — 接收指示
- `OSEKNM_EVENT_TXCONF` (0x08) — 发送确认

**核心状态转换（源码追踪）：**

1. **Bus-Sleep → Repeat Message**:
   - 触发: `PassiveStartUp` 或 `NetworkRequest` 设置 `EVENT_REQUEST`
   - 函数: `OsekNm_ResetEnter()` (line 1536)
   - 行为: 清除 status bits, 设置 `RINGDATA_NOTALLOWED`, 停止所有 timer, 重置 config, 递增 rxCount, 发送 Alive 消息

2. **Repeat Message → Normal Operation**:
   - 触发: RxCount/RxCount 达到限制，配置稳定
   - 行为: 进入 NORMAL_OPERATION 状态，开始周期性发送 Ring 消息

3. **Normal Operation → Prepare Bus-Sleep**:
   - 触发: `NetworkRelease` + 所有节点释放
   - 函数: `OsekNm_PrepareBusSleepEnter()` (line 2173)
   - 行为: 启动 TWbs 定时器

4. **Prepare Bus-Sleep → Bus-Sleep**:
   - 触发: TWbs 超时
   - 函数: `OsekNm_BusSleepEnter()` (line 1492)
   - 行为: 重置计数器, 通知 `Nm_StateChangeNotification`, 设置 BUSSLEEP 状态

5. **任意状态 → LimpHome**:
   - 触发: TError 超时
   - 行为: 设置 `OSEKNM_STATUS_LIMPHOME`，周期性发送 LimpHome 消息

6. **LimpHome → Repeat Message**:
   - 触发: 收到 Ring 消息（配置恢复）
   - 函数: `OsekNm_ResetEnter()` (line 1536)
   - 行为: 清除 LimpHome 状态, 重置计数器

#### 6AQ0 定时器常量

| 定时器 | 宏 | 用途 | 回调函数 |
|--------|-----|------|----------|
| TTyp | `OSEKNM_CFG_TTYP` | 典型 Ring 消息间隔 | OsekNm_TTyp |
| TMax | `OSEKNM_CFG_TMAX` | 最大 Ring 消息间隔 | OsekNm_TMax |
| TError | `OSEKNM_CFG_TERROR` | 错误检测超时 | OsekNm_TError |
| TWbs | `OSEKNM_CFG_TWBS` | 等待 Bus-Sleep | OsekNm_BusSleepEnter |
| TTx | `OSEKNM_CFG_TTX` | 发送重试间隔 | (动态设置) |
| TLimpErr | `OSEKNM_CFG_TLIMPERR` | LimpHome 错误时间 | (仅 DEM) |
| TTypOffset | `OSEKNM_CFG_TTYPOFFSET` | 总线负载降低偏移 | (仅 BusLoadReduction) |

### 1.2.2 6ER1 Direct NM 状态机 (OsekNm_ConfigTypes.h + OsekDirectNm.c)

**状态枚举** (10 个状态):
```
OSEKNM_OFF → OSEKNM_INIT → OSEKNM_INITRESET
                          → OSEKNM_NORMAL → OSEKNM_NORMALPREPSLEEP
                                          → OSEKNM_TwbsNORMAL
                          → OSEKNM_BUSSLEEP
                          → OSEKNM_LIMPHOME → OSEKNM_LIMPHOMEPREPSLEEP
                                            → OSEKNM_TwbsLIMPHOME
                          → OSEKNM_ON
```

**内部数据结构** (`OsekDirect_InternalType`, OsekDirectNm.h line 56-73):

```c
typedef struct {
    OsekNm_PduType nmTransmitPdu;                           // 自己发送的 NM PDU
    OsekNm_PduType nmRxPdu[OSEKNM_DIRECT_MAX_NODE_NUM];     // 接收到的 NM PDU（最多 8 个节点）
    NetworkStatusType networkStatus;                         // 网络状态 bitfield
    OsekNMMerkerType nmMerker;                              // {stable, limphome} 标记
    OseknmDirectTimer timer;                                // 定时器值
    OsekNm_CmaskParamsType nmDirectMask;                     // 配置掩码
    OsekNm_ConfigParamsType nmDirectConfig;                  // 节点配置表
    OsekNmDirectNmStateType nmState;                         // 当前状态
    uint8 nmRxRetryCounter;                                  // RxCount
    uint8 nmTxRetryCounter;                                  // TxCount
    uint8 nmRxPduCount;                                      // 接收 PDU 计数
    boolean rcvdNmpdu;                                       // 收到 NM PDU 标志
    boolean transNmpdu;                                      // 发送 NM PDU 标志
} OsekDirect_InternalType;
```

**定时器结构** (`OseknmDirectTimer`):
| 字段 | 对应 OSEK 定时器 | 说明 |
|------|------------------|------|
| `osekdirectNmTTx` | TTx | 发送重试间隔 |
| `osekdirectNmTTyp` | TTyp | 典型 Ring 间隔 |
| `osekdirectNmTMax` | TMax | 最大 Ring 间隔 |
| `osekdirectNmTError` | TError | LimpHome 超时 |
| `osekdirectNmTWbs` | TWbs | 等待 Bus-Sleep |
| `cfmotoTSleepRequestMin` | (CFMOTO 自定义) | Sleep 重请求时间 |
| `cfmotoTLimpSleepReqMin` | (CFMOTO 自定义) | LimpHome Sleep 请求 |
| `cfmotoTLimpHomeDTC` | (CFMOTO 自定义) | LimpHome DTC 记录 |

**每个状态有独立的 MainFunc 处理函数:**
- `normalMainFunc()` — NORMAL 状态主循环
- `normalPrepSleepMainFunc()` — NORMALPREPSLEEP 状态
- `normalTwbsMainFunc()` — TwbsNORMAL 状态
- `twbsLimphomeMainFunc()` — TwbsLIMPHOME 状态
- `limphomePrepSleepMainFunc()` — LIMPHOMEPREPSLEEP 状态
- `limphomeMainFunc()` — LIMPHOME 状态

**消息处理:**
- `rxNmpduProcessFunc()` — 正常 NM PDU 处理
- `rxNmpduLimpHomemsg()` — LimpHome 消息处理
- `rxNmpduSleepAckProc()` — Sleep ACK 处理
- `txNmpduProcessFunc()` — 发送处理

### 1.2.3 6ER1 Indirect NM 状态机 (OsekNm_ConfigTypes.h)

```
OSEKINDNM_OFF → OSEKINDNM_INIT → OSEKINDNM_AWAKE
                                → OSEKINDNM_BUSSLEEP
                                → OSEKINDNM_NORMAL
                                → OSEKINDNM_WAITBUSSLEEP
                                → OSEKINDNM_LIMPHOME
                                → OSEKINDNM_ON
```

当前 6ER1 配置 `OSEKNM_INDIRECT_NET_NUM = 0`，Indirect NM 代码已编译但未启用。

### 1.2.4 状态机对比总结

| 维度 | 6AQ0 | 6ER1 |
|------|------|------|
| 状态机层数 | **2 层**（Nm 7 状态 + OsekNm 内部 bitfield 状态） | **1 层**（Direct NM 10 状态枚举） |
| 状态表示 | 上层用枚举 (Nm_StateType), 下层用 bitfield | 直接用枚举 (OsekNmDirectNmStateType) |
| 中间状态 | READY_SLEEP, SYNCHRONIZE | INITRESET, ON, NORMALPREPSLEEP, TwbsNORMAL |
| LimpHome 子状态 | 无（Limphome 是状态位） | LIMPHOME → LIMPHOMEPREPSLEEP → TwbsLIMPHOME |
| Prepare Sleep | 有独立 PREPARE_BUS_SLEEP 状态 | 有 NORMALPREPSLEEP / LIMPHOMEPREPSLEEP 两个 |
| 定时器管理 | 回调函数指针（cbk）+ 递减计数 | 递减计数 + per-state MainFunc 分发 |
| 消息处理 | 通过 OpCode 宏（ALIVE/RING/LIMPHOME 及 IND/ACK 变体） | 通过 OpCode bitfield（alive/ring/limphome/sleepInd/sleepAck） |

---

## 1.3 配置参数对比

### 1.3.1 6AQ0 Nm 层配置 (Nm_Cfg.h)

| 宏 | 当前值 | 类别 | 说明 |
|----|--------|------|------|
| `NM_PASSIVE_MODE_ENABLED` | `STD_OFF` | 模式 | Active 模式（可请求网络） |
| `NM_REMOTE_SLEEP_IND_ENABLE` | `STD_OFF` | 功能 | 远程睡眠指示 |
| `NM_REPEAT_MSG_IND_ENABLED` | `STD_OFF` | 功能 | 重复消息指示 |
| `NM_COORDINATOR_SUPPORT_ENABLED` | `STD_OFF` | 功能 | 协调器/网关支持 |
| `NM_BUS_SYNCHRONIZATION_ENABLED` | `STD_OFF` | 功能 | 总线同步 |
| `NM_COM_CONTROL_ENABLED` | `STD_ON` | 功能 | 通信控制 (Disable/Enable) |
| `NM_DEV_ERROR_DETECT` | `STD_OFF` | 调试 | 开发错误检测 |
| `NM_MULTIPLE_CHANNELS_ENABLED` | `STD_ON` | 通道 | 多通道支持 |
| `NM_NODE_ID_ENABLED` | `STD_ON` | 功能 | 节点 ID |
| `NM_NODE_DETECTION_ENABLED` | `STD_ON` | 功能 | 节点检测 (RepeatMessage) |
| `NM_USER_DATA_ENABLED` | `STD_ON` | 功能 | NM 消息携带用户数据 |
| `NM_PDU_RX_INDICATION_ENABLED` | `STD_ON` | 功能 | PDU 接收通知 |
| `NM_STATE_CHANGE_IND_ENABLED` | `STD_ON` | 功能 | 状态变更通知 |
| `NM_VERSION_INFO_API` | `STD_ON` | 调试 | 版本信息 API |
| `NM_BUSNM_CANNM_ENABLED` | `STD_OFF` | 总线选择 | CAN NM |
| `NM_BUSNM_FRNM_ENABLED` | `STD_OFF` | 总线选择 | FlexRay NM |
| `NM_BUSNM_LINNM_ENABLED` | `STD_OFF` | 总线选择 | LIN NM |
| `NM_BUSNM_OSEKNM_ENABLED` | `STD_ON` | **总线选择** | **OSEK NM（唯一启用的总线）** |
| `NM_NUMBER_OF_CHANNELS` | 1 | 通道 | 单通道 |

### 1.3.2 6AQ0 OsekNm 层配置 (OsekNm_Cfg.h)

| 宏 | 当前值 | 说明 |
|----|--------|------|
| `OSEKNM_DEM_ERROR_DETECT` | `STD_OFF` | DEM 错误检测 |
| `OSEKNM_DEV_ERROR_DETECT` | `STD_OFF` | 开发错误检测 |
| `OSEKNM_BUS_SYNCHRONIZATION_ENABLED` | `STD_OFF` | 总线同步 |
| `OSEKNM_COM_CONTROL_ENABLED` | `STD_ON` | COM 层控制 |
| `OSEKNM_IMMEDIATE_TXCONF_ENABLED` | `STD_OFF` | 立即 Tx 确认 |
| `OSEKNM_NUMBER_OF_CHANNELS` | 1 | 通道数 |
| `OSEKNM_CHANNEL_ID_OFFSET` | 0 | 通道 ID 偏移（NmIf ↔ OsekNm 转换） |
| `OSEKNM_PDU_RX_INDICATION_ENABLED` | `STD_OFF` | PDU 接收通知 |
| `OSEKNM_REMOTE_SLEEP_IND_ENABLED` | `STD_OFF` | 远程睡眠指示 |
| `OSEKNM_REPEAT_MSG_IND_ENABLED` | `STD_OFF` | 重复消息指示 |
| `OSEKNM_STATE_CHANGE_IND_ENABLED` | `STD_ON` | 状态变更通知 |
| `OSEKNM_USER_DATA_ENABLED` | `STD_ON` | 用户数据 |
| `OSEKNM_VERSION_INFO_API` | `STD_ON` | 版本信息 |
| `OSEKNM_BUSLOADREDUCTIONENABLED` | `STD_ON` | 总线负载降低 |
| `OSEKNM_RX_INTERRUPT` | `STD_OFF` | Rx 中断模式 |
| `OSEKNM_TX_INTERRUPT` | `STD_ON` | Tx 中断模式 |
| `OSEKNM_TX_PDU_PENDING_ENABLE` | `STD_ON` | Tx PDU 挂起 |
| `OSEKNM_TX_PDU_PENDING_VALUE` | `0x0` | Tx PDU 填充值 |

**消息类型定义:**
```c
#define OSEKNM_MSG_MASK      0x37u
#define OSEKNM_MSG_RING      0x2u     // Ring 消息
#define OSEKNM_MSG_RING_IND  0x12u    // Ring + SleepInd
#define OSEKNM_MSG_RING_ACK  0x22u    // Ring + SleepAck
#define OSEKNM_MSG_RING_IND_ACK 0x32u // Ring + SleepInd + SleepAck
#define OSEKNM_MSG_ALIVE     0x1u     // Alive 消息
#define OSEKNM_MSG_ALIVE_IND 0x11u    // Alive + SleepInd
#define OSEKNM_MSG_ALIVE_IND_ACK 0x31u // Alive + SleepInd + SleepAck
#define OSEKNM_MSG_LIMPHOME  0x4u     // LimpHome 消息
#define OSEKNM_MSG_LIMPHOME_IND 0x14u // LimpHome + SleepInd
#define OSEKNM_MSG_LIMPHOME_IND_ACK 0x34u // LimpHome + SleepInd + SleepAck
```

**CAN ID 范围:**
```c
#define OSEKNM_BASEADDR  0x400   // NM CAN ID 起始
#define OSEKNM_MAXADDR   0x4FF   // NM CAN ID 结束
```

### 1.3.3 6ER1 配置 (OsekNm_Cfg.h + OsekNm_Lcfg.c)

| 宏/参数 | 当前值 | 说明 |
|----------|--------|------|
| `OSEKNM_DIRECT_NET_NUM` | 1 | Direct NM 网络数量 |
| `OSEKNM_INDIRECT_NET_NUM` | 0 | Indirect NM 网络数量 |
| `OSEKNM_RX_COUNT_LIMIT` | 4 | Rx 重试限制 |
| `OSEKNM_TX_COUNT_LIMIT` | 8 | Tx 重试限制 |
| `OSEKNM_DLL_FRAME_LENGTH` | 8 | DLL 帧长度 (8 bytes) |
| `CPU_BYTE_ORDER` | 0 (LOW_BYTE_FIRST) | 字节序 |
| `OSEKNM_DIRECT_STATE_CHANGE_INDICATION` | `STD_ON` | 状态变更回调 |
| `ASR_OSEK_NM` | `STD_OFF` | ASR OSEK NM（未启用） |
| `OSEK_MAIN_LOOP_PERIOD` | 5 (ms) | 主循环周期 |
| NodeId | 0x02 | 本地节点 ID |
| TTx | 1 tick (5ms) | 发送重试间隔 |
| TTyp | 100 ticks (500ms) | 典型 Ring 间隔 |
| TMax | 260 ticks (1300ms) | 最大 Ring 间隔 |
| TError | 1000 ticks (5000ms) | 错误超时 |
| TWbs | 1000 ticks (5000ms) | 等待 Bus-Sleep |
| cfmotoNmTSleepRequest | 900 ticks (4500ms) | CFMOTO Sleep 重请求 |
| cfmotoLimpHomeTSleepRequest | 900 ticks (4500ms) | CFMOTO LimpHome Sleep |
| cfmotoLimpHomeDTC | 500 ticks (2500ms) | CFMOTO LimpHome DTC |
| TXPDU_ID | `TXPDU_ID_NM_402` | CAN Tx PDU 引用 |

### 1.3.4 配置参数映射表

| 功能域 | 6AQ0 | 6ER1 | 映射关系 |
|--------|------|------|----------|
| NM 模式 | `NM_PASSIVE_MODE_ENABLED` | 无对应 | 6ER1 通过 GotoMode 切换 |
| 通信控制 | `NM_COM_CONTROL_ENABLED` + `OSEKNM_COM_CONTROL_ENABLED` | `CanNm_CommCtl()` | 功能等价，API 不同 |
| 用户数据 | `NM_USER_DATA_ENABLED` + `OSEKNM_USER_DATA_ENABLED` | 无独立开关 | 6ER1 通过 TransmitRingData |
| 节点 ID | `NM_NODE_ID_ENABLED` | 无开关 (始终启用) | 6ER1 在配置结构体中 |
| 节点检测 | `NM_NODE_DETECTION_ENABLED` | 无独立功能 | 6ER1 无 RepeatMessageRequest |
| 状态变更通知 | `NM_STATE_CHANGE_IND_ENABLED` + `OSEKNM_STATE_CHANGE_IND_ENABLED` | `OSEKNM_DIRECT_STATE_CHANGE_INDICATION` | **近似等价** |
| PDU 接收通知 | `NM_PDU_RX_INDICATION_ENABLED` | 无开关 | 6ER1 始终通过 RxIndication |
| 总线负载降低 | `OSEKNM_BUSLOADREDUCTIONENABLED` | 无此功能 | **6ER1 缺失** |
| Tx 中断模式 | `OSEKNM_TX_INTERRUPT` | 无此选项 | 6ER1 统一用轮询 |
| Rx 中断模式 | `OSEKNM_RX_INTERRUPT` | 无此选项 | 同上 |
| 立即 TxConf | `OSEKNM_IMMEDIATE_TXCONF_ENABLED` | 无此选项 | 6ER1 始终走 TxConfirmation |
| Tx 重试限制 | `OSEKNM_CFG_TX_LIMIT` | `OSEKNM_TX_COUNT_LIMIT` | **功能等价** |
| Rx 重试限制 | `OSEKNM_CFG_RX_LIMIT` | `OSEKNM_RX_COUNT_LIMIT` | **功能等价** |
| TTx | `OSEKNM_CFG_TTX` | `osekdirectNmTTx` | **等价** |
| TTyp | `OSEKNM_CFG_TTYP` | `osekdirectNmTTyp` | **等价** |
| TMax | `OSEKNM_CFG_TMAX` | `osekdirectNmTMax` | **等价** |
| TError | `OSEKNM_CFG_TERROR` | `osekdirectNmTError` | **等价** |
| TWbs | `OSEKNM_CFG_TWBS` | `osekdirectNmTWbs` | **等价** |
| CFMOTO Sleep | 使用 SleepRequestMinL_Cnt 全局变量 | `cfmotoTSleepRequestMin` / `cfmotoTLimpSleepReqMin` | **功能等价**，6ER1 更规范 |
| LimpHome DTC | LimpHomeDTC_Cnt 全局变量 | `cfmotoTLimpHomeDTC` | **功能等价**，6ER1 更规范 |

---

## 1.4 数据结构对比

### 1.4.1 6AQ0 核心数据结构

**每个通道的上下文** (`OsekNm_ChannelType`, OsekNm.c line 148-174):

```c
typedef struct {
    OsekNm_TimerType tStd;          // 标准定时器 (TTyp/TMax/TError/TWbs)
    OsekNm_TimerType tTx;           // 发送定时器 (TTx)
    OsekNm_StatusType status;       // 状态 bitfield (16 位有效)
    uint8 txCnt;                    // TxCount
    uint8 rxCnt;                    // RxCount
    OsekNm_ConfigRefType cfgNormal[32];    // 正常配置表（256 节点 bitmask）
    OsekNm_ConfigRefType cfgLimpHome[32];  // LimpHome 配置表
    Nm_StateType state;             // Nm 层状态（7 状态枚举）
    Nm_ModeType mode;               // Nm 层模式（4 模式枚举）
    OsekNm_EventType event;         // 事件标志 (REQUEST|TXFLAG|RXIND|TXCONF)
    uint8 txBuf[9];                 // Tx PDU 缓冲区
    uint8 rxBuf[10];                // Rx PDU 缓冲区
} OsekNm_ChannelType;

// 全局数组
OsekNm_ChannelType OsekNm_ChannelData[OSEKNM_NUMBER_OF_CHANNELS]; // 1 个通道
```

**定时器结构:**
```c
typedef struct {
    OsekNm_CbkFuncType cbk;     // 超时回调函数指针
    uint16_least timer;         // 当前计数值
} OsekNm_TimerType;
```

**PDU 格式** (CAN ID 范围 0x400-0x4FF):
```
Tx Buffer (9 bytes):  [DestId(1)] [OpCode(1)] [RingData(6)] [RingDataLen(1)]
Rx Buffer (10 bytes): [DestId(1)] [OpCode(1)] [RingData(6)] [RingDataLen(1)] [SrcId(1)]
```
- 消息类型通过 CAN ID 编码（0x400-0x4FF 范围内）
- PDU 内部 OpCode 用于区分 Alive/Ring/LimpHome 及 SleepInd/SleepAck

### 1.4.2 6ER1 核心数据结构

**每个 Direct NM 网络的上下文** (`OsekDirect_InternalType`, OsekDirectNm.h line 56-73):

```c
typedef struct {
    OsekNm_PduType nmTransmitPdu;                              // 本地发送 PDU
    OsekNm_PduType nmRxPdu[OSEKNM_DIRECT_MAX_NODE_NUM];       // 接收 PDU 环（8 节点）
    NetworkStatusType networkStatus;                            // 32-bit 网络状态
    OsekNMMerkerType nmMerker;                                 // {stable, limphome}
    OseknmDirectTimer timer;                                   // 8 个定时器字段
    OsekNm_CmaskParamsType nmDirectMask;                       // 配置掩码 (256 节点)
    OsekNm_ConfigParamsType nmDirectConfig;                    // 节点配置表 (256 节点)
    OsekNmDirectNmStateType nmState;                           // 当前状态枚举
    uint8 nmRxRetryCounter;                                    // RxCount
    uint8 nmTxRetryCounter;                                    // TxCount
    uint8 nmRxPduCount;                                        // 接收到的 PDU 数
    boolean rcvdNmpdu;                                         // 是否收到 NM PDU
    boolean transNmpdu;                                        // 是否发送 NM PDU
} OsekDirect_InternalType;

// 全局数组
static OsekDirect_InternalType OsekDirectNm_Networks[OSEKNM_DIRECT_NET_NUM]; // 1 个网络
```

**PDU 格式** (取决于字节序):
```
LOW_BYTE_FIRST:  [RingData(6)] [OpCode(1)] [Destination(1)] [Source(1)] = 9 bytes
HIGH_BYTE_FIRST: [Source(1)] [Destination(1)] [OpCode(1)] [RingData(6)] = 9 bytes
```

OpCode bitfield:
| bit | 名称 | 含义 |
|-----|------|------|
| 0 | alive | Alive 消息 |
| 1 | ring | Ring 消息 |
| 2 | limphome | LimpHome 消息 |
| 3 | reserved1 | 保留 |
| 4 | sleepInd | 睡眠指示 |
| 5 | sleepAck | 睡眠确认 |
| 6-7 | reserved2 | 保留 |

### 1.4.3 PDU 格式对比

| 维度 | 6AQ0 | 6ER1 |
|------|------|------|
| 消息类型识别 | **CAN ID** (0x400-0x4FF 范围) | **PDU OpCode** (bitfield) |
| PDU 大小 | 9 bytes (Tx) / 10 bytes (Rx, 含 SrcId) | 9 bytes |
| 源地址 | Rx 中有独立 SrcId 字段 | PDU 中有 Source 字段 |
| 目标地址 | PDU 中有 DestId | PDU 中有 Destination 字段 |
| Sleep 指示 | OpCode 变体: SleepInd=0x10, SleepAck=0x20 | OpCode bit 4=sleepInd, bit 5=sleepAck |
| 字节序依赖 | 固定 | 通过 `CPU_BYTE_ORDER` 配置 |

**关键结论: 两种 PDU 格式在线缆层面不兼容**。6AQ0 通过 CAN ID 区分 NM 信道/功能，6ER1 通过 PDU 内容区分。

### 1.4.4 内存占用估算

| 项目 | 单通道上下文大小估算 |
|------|---------------------|
| 6AQ0 `OsekNm_ChannelType` | ~200 bytes (2 timer structs × 6B + 64B config × 2 + 19B buffers + 控制字段) |
| 6ER1 `OsekDirect_InternalType` | ~180 bytes (9B PDU × 9 + 32B timer + 64B config × 4 + 控制字段) |

两者单通道内存占用相当（~200 bytes）。

---

## 1.5 集成点分析

### 1.5.1 6AQ0 完整调用链

```
应用层
  │
  ▼
ComM (ComM.c 87KB)
  │ ComM_Nm_NetworkRequest(Channel)
  │ ComM_Nm_NetworkRelease(Channel)
  ▼
Nm 层 (Nm.c 78KB) ─── Nm_Cfg.h 配置
  │ Nm_NetworkRequest() → 查找 Nm_Channels[] → 路由到对应 BusNm
  │ (当前仅启用 OSEKNM)
  ▼
OsekNm 层 (OsekNm.c 103KB) ─── OsekNm_Cfg.h 配置
  │ OsekNm_NetworkRequest() → 设置 EVENT_REQUEST 标志
  │ OsekNm_MainFunction() → 定时器递减 + 事件处理 + 状态机驱动
  │ OsekNm_SendMessage() → 构造 PDU → CanIf 发送
  ▼
CanIf (CAN Interface)
  │ CanIf_Transmit(PduId, PduInfo)
  ▼
CAN Driver (BSP/canb.c)
  │ 硬件发送
  ▼
CAN Bus
```

**回调链（从下到上）:**
```
CAN Driver (Rx Interrupt / Tx Complete)
  │
  ▼
CanIf (CanIf_RxIndication / CanIf_TxConfirmation)
  │
  ▼
OsekNm 层
  │ OsekNm_RxIndication() → 设置 EVENT_RXIND
  │ OsekNm_TxConfirmation() → 设置 EVENT_TXCONF
  │ (在 MainFunction 中处理事件)
  │ 状态变化时:
  ▼
Nm 层
  │ Nm_StateChangeNotification(prevState, curState)
  │ Nm_NetworkMode(nmNetworkHandle)
  │ Nm_BusSleepMode(nmNetworkHandle)
  │ ...
  ▼
ComM
  │ ComM_Nm_NetworkMode()
  │ ComM_Nm_BusSleepMode()
  │ ...
```

**调度方式:**
- `OsekNm_MainFunction()` 由 SchM (Schedule Manager) 周期性调用
- SchM 提供互斥保护 (`SchM_Enter_OsekNm` / `SchM_Exit_OsekNm`) 用于 TxBuf/RxBuf/Status 访问
- 定时器粒度: MainFunction 每调用一次，所有 active timer 减 1

### 1.5.2 6ER1 完整调用链

```
应用层 / CanSM
  │
  ▼
OsekNm 层 (OsekNm.c 22KB)
  │ OsekNm_NetworkRequest()
  │ OsekNm_MainFunction() → 分发到:
  │   ├── OsekDirectNm_MainFunction(netId)   [OSEKNM_DIRECT_NET_NUM > 0]
  │   └── (OsekIndirectNm_MainFunction)       [OSEKNM_INDIRECT_NET_NUM > 0，当前为 0]
  ▼
OsekDirectNm 层 (OsekDirectNm.c 70KB)
  │ 根据 nmState 调用 per-state MainFunc:
  │   normalMainFunc() / normalPrepSleepMainFunc() / limphomeMainFunc() / ...
  │ sendNmMessage() → D_Window_Data_req() → CanIf
  ▼
CanIf (CAN Interface)
  │
  ▼
CAN Driver
  │
  ▼
CAN Bus
```

**回调链（从下到上）:**
```
CAN Driver
  │
  ▼
CanIf
  │ OsekNm_TxConfirmation(NetId, NodeId)
  │ OsekNm_RxIndication(NetId, NodeId, PDU)
  ▼
OsekNm 层
  │ 分发到:
  │   OsekDirectNm_TxConformation(netId)
  │   OsekDirectNm_RxIndication(netId, nmPdu)
  ▼
OsekDirectNm 层
  │ rxNmpduProcessFunc() / txNmpduProcessFunc()
  │ 状态变化时: STATE_CHANGE_INDICATION(state) 宏
  ▼
应用层回调 (通过 OsekNm_ConfigType 中的函数指针):
  OsekNmDirectULStateChangeCallback(netId, state)
```

**调度方式:**
- `OsekNm_MainFunction()` 由应用层周期性调用（基于 OSEK_MAIN_LOOP_PERIOD = 5ms）
- 无 SchM 互斥保护（依赖 FreeRTOS 协作调度，`configUSE_PREEMPTION = 0`）
- 定时器值以 MainFunction 调用次数为单位

### 1.5.3 回调注册机制对比

| 维度 | 6AQ0 | 6ER1 |
|------|------|------|
| 注册方式 | 编译时通过 `extern` 声明链接（`Nm_Cbk.h` 中声明，ComM 实现） | 运行时通过 `OsekNm_ConfigType` 结构体中的函数指针 |
| 回调类型 | 9 个独立的回调函数 | 2 个回调函数指针（Direct + Indirect） |
| 状态变更 | 传递 prev + current 两个状态 | 传递 current 状态 |
| 数据通知 | 独立回调: `Nm_PduRxIndication` | 通过公共 API: `OsekNm_RxIndication` |
| 灵活性 | 低（编译时绑定） | 高（运行时配置） |
| AUTOSAR 合规 | ✅（符合 AUTOSAR Nm 回调规范） | ❌（简化的 OSEK 风格） |

### 1.5.4 TxConfirmation / RxIndication 数据流

**6AQ0:**
```
Tx: OsekNm_SendMessage → CanIf_Transmit → CAN 硬件 → Tx 中断
    → CanIf_TxConfirmation → OsekNm 设置 EVENT_TXCONF → MainFunction 中处理

Rx: CAN 硬件 → Rx 中断 → CanIf_RxIndication → OsekNm 设置 EVENT_RXIND
    → MainFunction 中 OsekNm_RxIndProcess() → 检查 OpCode → 更新状态机
```

**6ER1:**
```
Tx: sendNmMessage → D_Window_Data_req → CanIf → CAN 硬件 → Tx 完成
    → OsekNm_TxConfirmation → OsekDirectNm_TxConformation
    → txNmpduProcessFunc() → 更新 TxCount → 可能触发下一条消息

Rx: CAN 硬件 → CanIf → OsekNm_RxIndication → OsekDirectNm_RxIndication
    → 解码 OpCode → rxNmpduProcessFunc() / rxNmpduLimpHomemsg() / rxNmpduSleepAckProc()
    → 更新 RxCount / 更新配置表 / 驱动状态转换
```

### 1.5.5 集成方式差异总结

| 维度 | 6AQ0 | 6ER1 |
|------|------|------|
| 上层调用方 | ComM (Communication Manager) | CanSM / 应用直接调用 |
| NM 模块与 CAN 栈关系 | 通过 CanIf (PduR 路由) | 通过 CanIf + DLL 抽象层 (D_* 函数) |
| 总线控制 | CanNm_CommCtl (内部) | D_Online/D_Offline + BusInit/BusSleep |
| 配置路径 | Nm_Cfg.h → OsekNm_Cfg.h → OsekNm_Cfg.c (宏链) | OsekNm_Cfg.h → OsekNm_Lcfg.c → OsekNm_ConfigType (结构体链) |
| 平台抽象 | SchM (Schedule Manager) + MemMap (Memory Mapping) | FreeRTOS 协作调度 + 标准 C 内存 |
| 编译器依赖 | ARMCC V5 (Keil 特有 pragma: `#define NM_START_SEC_CODE`) | arm-none-eabi-gcc 10 (标准 C，无特殊 pragma) |

---

## 附录 A: 函数索引（源码行号）

### 6AQ0 OsekNm.c 关键函数

| 行号 | 函数 | 类型 |
|------|------|------|
| 579 | `OsekNm_PassiveStartUp` | API |
| 618 | `OsekNm_NetworkRequest` | API |
| 657 | `OsekNm_NetworkRelease` | API |
| 706 | `OsekNm_DisableCommunication` | API |
| 748 | `OsekNm_EnableCommunication` | API |
| 790 | `OsekNm_SetUserData` | API |
| 835 | `OsekNm_GetUserData` | API |
| 883 | `OsekNm_GetNodeIdentifier` | API |
| 915 | `OsekNm_GetLocalNodeIdentifier` | API |
| 945 | `OsekNm_RepeatMessageRequest` | API |
| 972 | `OsekNm_GetPduData` | API |
| 1008 | `OsekNm_GetState` | API |
| 1069 | `OsekNm_CheckRemoteSleepIndication` | API |
| 1302 | `OsekNm_BusOff` | API |
| 1343 | `OsekNm_BusOffRecovery` | API |
| 1385 | `OsekNm_LimpHomeDTCReset` | API |
| 1390 | `OsekNm_MainFunction` | 核心 |
| 1492 | `OsekNm_BusSleepEnter` | 状态转换 |
| 1536 | `OsekNm_ResetEnter` | 状态转换 |
| 2015 | `OsekNm_LimpHomeStandard` | 状态转换 |
| 2173 | `OsekNm_PrepareBusSleepEnter` | 状态转换 |
| 2570 | `OsekNm_SendMessage` | 消息发送 |

---

> **文档版本**: V1.0 | **日期**: 2026-05-27 | **作者**: Claude Code Agent
>
> **下一步**: 🟦 Codex Agent 评审本报告，确认无误后 Phase 1 门禁通过，进入 Phase 2（标准化设计）。
