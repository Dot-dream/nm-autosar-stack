# AUTOSAR NM 测试用例

> **测试目标**: 验证 NM 模块 AUTOSAR NM（广播 + CBV + 协调器）状态机的网络唤醒与休眠行为。
>
> **测试环境**: USB2CAN (UTA0503) + Demo_Project (YTM32B1MC0)
>
> **NM 配置**: `NM_TEST_MODE = 2`，CAN ID 0x402 (同 OSEK 共用)
>
> **定时器**: timerTyp=1000ms (NM PDU 周期), timerMax=2000ms (NM_TIMEOUT_TIMER), timerWaitBusSleep=2000ms
>
> **CBV (Control Bit Vector) 字段定义** (按实际实现):
>
> | Bit | 名称 | 含义 |
> |:---:|------|------|
> | 0 | RepeatMessageRequest | REPEAT_MESSAGE 状态下 =1；NORMAL_OPERATION 下 =0 |
> | 1 | ActiveWakeup | NetworkRequest 后 =1（表示节点主动需要网络）；进入 READY_SLEEP 后 =0 |
> | 2 | PNI | Partial Network Info（当前未实现 PNI 过滤，编解码已就位） |
> | 3 | CoordinatorSleep | 协调器 SYNCHRONIZE 状态下 =1 |
> | 4-5 | UserDataLen | 用户数据长度 (0-6) |
> | 6-7 | Reserved | 保留 |
>
> **UART 日志格式说明**:
> - 状态变化回调输出: `NM [ch0] State: <prev> -> <cur>`
> - 判定标准中简写为 `NM: <事件描述>`，实际需匹配上述格式
>
> ⚠️ **测试范围说明**:
> - 因测试环境限制，**不包含 K15 点火唤醒相关测试**。仅验证网络唤醒（收到 NM PDU 唤醒）与休眠流程。
> - **PNI (Partial Network Information) 过滤** 当前仅在 CBV 编解码层面实现，DUT 侧无 PNI 匹配逻辑，用例 8 标记为待实现。
> - **多通道共存** (三协议并行) 需 `numChannels > 1`，当前 Demo 为单通道，用例 9 标记为待实现。

---

## 用例 1: 基本生命周期 — BUS_SLEEP → REPEAT_MESSAGE → NORMAL_OPERATION → READY_SLEEP → PREPARE_BUS_SLEEP → BUS_SLEEP

**前置条件**: DUT 上电，AUTOSAR NM 初始化完成。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 1.1 | DUT 上电 | DUT 进入 BUS_SLEEP | UART: `NM [ch0] State: UNINIT -> BUS_SLEEP` |
| 1.2 | 调用 `Nm_NetworkRequest(0)` | DUT 进入 REPEAT_MESSAGE，开始周期性广播 NM PDU | PCAN 捕获 CAN ID 0x402，CBV: Bit0=1 (RepeatMsgReq), Bit1=1 (ActiveWakeup)。UART: `NM [ch0] << NetworkStart (wakeup)` |
| 1.3 | 等待 ~3s（Repeat Message 持续约 `txCountLimit × timerTyp = 8s`，此处按实际观测） | DUT 自动进入 NORMAL_OPERATION | UART: `NM [ch0] << NetworkMode` → `NM [ch0] State: REPEAT_MSG -> NORMAL_OP`。CBV 变为 Bit0=0, Bit1=1 |
| 1.4 | DUT 在 NORMAL_OPERATION，周期性发送 NM PDU | 每条 NM PDU 间隔 ~1s | PCAN 持续捕获，间隔 ~1000ms ± 200ms |
| 1.5 | 调用 `Nm_NetworkRelease(0)` | DUT 进入 READY_SLEEP，CBV 中清除 ActiveWakeup | UART: `NM [ch0] State: NORMAL_OP -> READY_SLEEP`。PCAN: CBV Bit1=0 (ActiveWakeup 清除) |
| 1.6 | 等待 timerWaitBusSleep (2000ms) | DUT 进入 PREPARE_BUS_SLEEP | UART: `NM [ch0] << PrepareBusSleep` |
| 1.7 | 继续等待 timerWaitBusSleep | DUT 进入 BUS_SLEEP，停止发送 NM PDU | UART: `NM [ch0] << BusSleep`。PCAN 静默 |

---

## 用例 2: CBV 字段编码验证

**前置条件**: DUT 处于 REPEAT_MESSAGE 或 NORMAL_OPERATION 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 2.1 | DUT 在 REPEAT_MESSAGE | 发出 NM PDU | PCAN 捕获，解析 CBV (Data[1]) |
| 2.2 | 验证 REPEAT_MESSAGE 阶段 CBV | Bit0=1 (RepeatMsgReq), Bit1=1 (ActiveWakeup), Bit2=0, Bit3=0 | 解析值与预期一致 |
| 2.3 | DUT 进入 NORMAL_OPERATION | CBV 变化 | Bit0=0 (RepeatMsgReq 清除), Bit1=1 (ActiveWakeup 保持) |
| 2.4 | 调用 `Nm_RepeatMessageRequest(0)` | 下一条 NM PDU 的 CBV Bit0=1 | 仅一条消息携带 RepeatMsgReq，随后恢复 0（由 `Nm_NetworkRequest` 内部触发 `cbvRepeatMsgReq=1`） |
| 2.5 | DUT 进入 READY_SLEEP (调用 Nm_NetworkRelease) | CBV Bit1=0 (ActiveWakeup 清除) | 所有 NM PDU 中 Bit1=0 |

---

## 用例 3: 被动唤醒 — 收到远程 NM PDU（网络唤醒）

**前置条件**: DUT 处于 BUS_SLEEP。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 3.1 | DUT 在 BUS_SLEEP | 无 NM 消息发送 | PCAN 静默 > 3s |
| 3.2 | PCAN 发送 AUTOSAR NM PDU (CAN ID 0x402, Data[0]=发送者NodeID, Data[1]=0x03 — Bit0=1+Bit1=1) | DUT 被动唤醒，进入 REPEAT_MESSAGE | UART: `NM [ch0] << NetworkStart (wakeup)` → `NM [ch0] State: BUS_SLEEP -> REPEAT_MSG` |
| 3.3 | 等待 Repeat Message 结束 | DUT 进入 NORMAL_OPERATION | PCAN 捕获到 DUT 的周期性 NM PDU |

---

## 用例 4: 睡眠中止 (Abort Sleep)

**前置条件**: DUT 在 READY_SLEEP 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 4.1 | DUT 进入 READY_SLEEP (通过 Nm_NetworkRelease) | 周期性发送 NM PDU，CBV Bit1=0 | PCAN 可见 |
| 4.2 | 调用 `Nm_NetworkRequest(0)` | DUT 中止睡眠，回到 NORMAL_OPERATION | UART: `NM [ch0] State: READY_SLEEP -> NORMAL_OP` |
| 4.3 | 继续运行 | DUT 保持 NORMAL_OPERATION | PCAN: CBV Bit1=1，周期性 NM PDU 正常 |

---

## 用例 5: READY_SLEEP 中被远程节点拉回 NORMAL

**前置条件**: DUT 在 READY_SLEEP 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 5.1 | DUT 在 READY_SLEEP | CBV Bit1=0 | PCAN 可见 |
| 5.2 | PCAN 发送 NM PDU (CBV Bit0=1 或 Bit1=1，模拟远程节点请求网络) | DUT 检测到远程唤醒请求，回到 NORMAL_OPERATION | UART: `NM [ch0] State: READY_SLEEP -> NORMAL_OP` |
| 5.3 | 继续运行 | DUT 恢复周期性 NM PDU，CBV Bit1=1 | 正常 |

---

## 用例 6: 协调器 SYNCHRONIZE 状态

**前置条件**: DUT 配置 `busSyncEnabled=1`（协调器模式）。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 6.1 | DUT 在 NORMAL_OPERATION，调用 `Nm_NetworkRelease(0)` | 协调器进入 SYNCHRONIZE，CBV Bit3=1 (CoordinatorSleep) | UART: `NM [ch0] State: NORMAL_OP -> SYNCHRONIZE`。PCAN: CBV Bit3=1 |
| 6.2 | 等待 timerWaitBusSleep | DUT 进入 PREPARE_BUS_SLEEP | UART: `NM [ch0] << PrepareBusSleep` |
| 6.3 | 继续等待 | DUT 进入 BUS_SLEEP | UART: `NM [ch0] << BusSleep` |

---

## 用例 7: NM PDU 格式验证

**前置条件**: DUT 在 NORMAL_OPERATION。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 7.1 | 捕获 DUT 发送的 NM PDU | CAN ID = 0x402 | 固定 |
| 7.2 | 解析 PDU 结构 | Byte0=SourceNodeId, Byte1=CBV, Byte2..7=UserData | SourceNodeId=0x0A (与配置一致)，UserData 默认 0x00 |
| 7.3 | 检查 DLC | DLC = 8 | 固定 8 字节 |
| 7.4 | 验证周期性 | 相邻两条 NM PDU 间隔 ≈ timerTyp (1000ms) | 连续记录 10 条，均值 ≈ 1000ms，偏差 < 200ms |

---

## 用例 8: PNI (Partial Network Information) 预留验证

> ⚠️ **待实现**: PNI 过滤逻辑当前仅完成 CBV 编解码，DUT 侧无 PNI 位匹配与过滤。实现后执行。

**前置条件**: PNI 逻辑已实现。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 8.1 | DUT 在 BUS_SLEEP | — | — |
| 8.2 | PCAN 发送 NM PDU (ActiveWakeup=1, PNI bit=1, PNI 位不匹配) | DUT 不唤醒 | DUT 保持 BUS_SLEEP |
| 8.3 | PCAN 发送 NM PDU (ActiveWakeup=1, PNI bit=1, PNI 位匹配) | DUT 唤醒 | UART: `NM [ch0] State: BUS_SLEEP -> REPEAT_MSG` |

---

## 用例 9: 三协议共存（综合测试）

> ⚠️ **待实现**: 需 `numChannels >= 3`，当前 Demo 为单通道。实现后执行。

**前置条件**: 三个通道 CH0=Direct (0x402), CH1=Indirect (监控 0x41C), CH2=AUTOSAR (0x402)。

| 步骤 | 操作 | 预期结果 |
|------|------|----------|
| 9.1 | 三个通道同时 NetworkRequest | 各自独立运行，互不干扰 |
| 9.2 | Direct 通道 Release → Sleep | AUTOSAR 和 Indirect 不受影响 |
| 9.3 | Indirect 通道超时 → Sleep | Direct 和 AUTOSAR 不受影响 |
| 9.4 | AUTOSAR 通道 Release → Sleep | Direct 和 Indirect 不受影响 |
