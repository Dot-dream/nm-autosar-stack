# OSEK Indirect NM 测试用例

> **测试目标**: 验证 NM 模块 OSEK Indirect NM（应用消息监控）状态机的网络唤醒与休眠行为。
>
> **测试环境**: USB2CAN (UTA0503) + Demo_Project (YTM32B1MC0)
>
> **NM 配置**: `NM_TEST_MODE = 1`
>
> **说明**: Indirect NM 不发送专用 NM PDU，通过监控应用 CAN 消息判断网络状态。
>
> **UART 日志格式说明**:
> - 状态变化回调输出: `NM [ch0] State: <prev> -> <cur>`
> - 判定标准中简写为 `NM: <事件描述>`，实际需匹配上述格式
>
> ⚠️ **测试范围说明**: 因测试环境限制，**不包含 K15 点火唤醒相关测试**。仅验证网络唤醒（收到应用消息唤醒）与休眠流程。

---

## 用例 1: 基本生命周期 — BUS_SLEEP → AWAKE → NORMAL → WAIT_BUS_SLEEP → BUS_SLEEP

**前置条件**: DUT 上电，Indirect NM 初始化完成。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 1.1 | DUT 上电，NM 初始化 | DUT 进入 Bus-Sleep 状态 | UART: `NM [ch0] State: UNINIT -> BUS_SLEEP` |
| 1.2 | 调用 `Nm_NetworkRequest(0)` | DUT 进入 AWAKE，开始监听应用消息 | UART: `NM [ch0] State: BUS_SLEEP -> NORMAL_OP`（AWAKE 映射为 NORMAL_OP） |
| 1.3 | PCAN 发送任意应用 CAN 消息 (CAN ID 0x41C) | DUT 收到消息，进入 NORMAL | UART: `NM [ch0] << NetworkMode` |
| 1.4 | PCAN 持续周期发送应用消息 (每 500ms) | DUT 保持 NORMAL | UART 无状态变化（连续 10s 稳定） |
| 1.5 | PCAN 停止发送消息 | DUT 超时进入 WAIT_BUS_SLEEP | UART: `NM [ch0] State: NORMAL_OP -> PREPARE_BUS_SLEEP`（WAITBUSSLEEP 映射为 PREPARE_BUS_SLEEP）（约 TMax=2000ms 后） |
| 1.6 | 继续等待 TWbs (2000ms) | DUT 进入 BUS_SLEEP | UART: `NM [ch0] << BusSleep` |

---

## 用例 2: 睡眠中唤醒（网络唤醒）

**前置条件**: DUT 处于 BUS_SLEEP。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 2.1 | DUT 处于 BUS_SLEEP | 无状态变化，无 NM PDU 发送 | PCAN 静默，UART 无输出 |
| 2.2 | PCAN 发送任意应用 CAN 消息 (CAN ID 0x41C) | DUT 唤醒，进入 AWAKE | UART: `NM [ch0] << NetworkStart (wakeup)` → `NM [ch0] State: BUS_SLEEP -> NORMAL_OP` |
| 2.3 | PCAN 继续发送消息 | DUT 进入 NORMAL | UART: `NM [ch0] << NetworkMode` |

---

## 用例 3: 消息间歇与超时

**前置条件**: DUT 处于 NORMAL 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 3.1 | DUT 在 NORMAL | 监控运行中 | — |
| 3.2 | PCAN 每 1500ms 发送一条消息 (间隔 < TMax=2000ms) | DUT 保持 NORMAL，不超时 | 观察 10s，无状态变化 |
| 3.3 | PCAN 改为每 2500ms 发送一条消息 (间隔 > TMax=2000ms) | DUT 超时，进入 WAIT_BUS_SLEEP | UART: `NM [ch0] State: NORMAL_OP -> PREPARE_BUS_SLEEP` |
| 3.4 | 在 WAIT_BUS_SLEEP 期间发送一条消息 | DUT 回到 NORMAL | UART: `NM [ch0] << NetworkMode` / `NM [ch0] State: PREPARE_BUS_SLEEP -> NORMAL_OP` |

---

## 用例 4: NetworkRequest / NetworkRelease 主动控制

**前置条件**: DUT 处于 BUS_SLEEP。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 4.1 | 调用 `Nm_NetworkRequest(0)` | DUT 进入 AWAKE | UART: `NM [ch0] State: BUS_SLEEP -> NORMAL_OP` |
| 4.2 | 等待 5s（无应用消息） | DUT 保持在 AWAKE（AWAKE 状态下不会因 ToB 自动超时） | UART 无状态变化 |
| 4.3 | 调用 `Nm_NetworkRelease(0)` | DUT 进入 WAIT_BUS_SLEEP | UART: `NM [ch0] State: NORMAL_OP -> PREPARE_BUS_SLEEP` |
| 4.4 | 等待 TWbs | DUT 进入 BUS_SLEEP | UART: `NM [ch0] << BusSleep` |

---

## 用例 5: Bus-Off 错误处理

**前置条件**: DUT 处于 NORMAL 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 5.1 | 调用 `Nm_ControllerBusOff(0)` | DUT 进入 LIMPHOME | UART: `NM [ch0] State: NORMAL_OP -> LIMPHOME` |
| 5.2 | PCAN 发送应用消息 | DUT 恢复 AWAKE | UART: `NM [ch0] State: LIMPHOME -> NORMAL_OP` |
