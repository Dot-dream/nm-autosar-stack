# OSEK Direct NM 测试用例

> **测试目标**: 验证 NM 模块 OSEK Direct NM（逻辑环）状态机的网络唤醒与休眠行为。
>
> **测试环境**: USB2CAN (UTA0503) + Demo_Project (YTM32B1MC0)
>
> **NM 配置**: `NM_TEST_MODE = 0`，CAN ID 0x402 (Tx), 0x402/0x403 (Rx)
>
> **定时器**: TTyp=1000ms, TMax=2000ms, TError=1000ms, TWbs=2000ms
>
> **UART 日志格式说明**:
> - 状态变化回调输出: `NM [ch0] State: <prev> -> <cur>`（如 `BUS_SLEEP -> INITRESET`）
> - 网络模式回调输出: `NM [ch0] << NetworkMode`
> - 唤醒回调输出: `NM [ch0] << NetworkStart (wakeup)`
> - 休眠回调输出: `NM [ch0] << BusSleep`
> - 判定标准中简写为 `NM: <事件描述>`，实际需匹配上述格式
>
> ⚠️ **测试范围说明**: 因测试环境限制，**不包含 K15 点火唤醒相关测试**。仅验证网络唤醒（收到 NM PDU 唤醒）与休眠流程。

---

## 用例 1: 基本生命周期 — Alive → Ring → Normal → Sleep

**前置条件**: DUT 上电，NM 模块初始化完成。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 1.1 | DUT 上电，NM 初始化 | DUT 进入 Bus-Sleep 状态 | UART: `NM [ch0] State: UNINIT -> BUS_SLEEP` |
| 1.2 | 调用 `Nm_NetworkRequest(0)` | DUT 发送 Alive 消息，进入 INITRESET | PCAN 捕获 CAN ID 0x402，Data[0]=0x01 (Alive OpCode)，Data[1]=0x0A (本节点 ID)；UART: `NM [ch0] State: BUS_SLEEP -> INITRESET` |
| 1.3 | 等待 ~1s (TTyp) | DUT 发送 Ring 消息，进入 Normal | PCAN 捕获 CAN ID 0x402，Data[0]=0x02 (Ring OpCode)；UART: `NM [ch0] << NetworkMode` → `NM [ch0] State: INITRESET -> NORMAL_OP` |
| 1.4 | 等待 ~3s | DUT 持续周期性发送 Ring 消息 | PCAN 每隔 ~1s 捕获到一条 Ring 消息，间隔偏差 < ±200ms |
| 1.5 | 调用 `Nm_NetworkRelease(0)` | DUT 进入 Prepare Bus-Sleep | UART: `NM [ch0] << PrepareBusSleep` |
| 1.6 | 等待 ~2s (TWbs) | DUT 进入 Bus-Sleep | UART: `NM [ch0] << BusSleep`，之后 PCAN 静默无 NM 消息 |

---

## 用例 2: 被动唤醒 — Bus-Sleep 中收到 NM 消息（网络唤醒）

**前置条件**: DUT 处于 Bus-Sleep 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 2.1 | DUT 处于 Bus-Sleep | 无 NM 消息发送 | PCAN 连续 3s 无任何 CAN 帧 |
| 2.2 | PCAN 发送 Alive 消息 (CAN ID 0x402, Data[0]=0x01, Data[1]=0x0B) | DUT 被动唤醒 | UART: `NM [ch0] << NetworkStart (wakeup)` |
| 2.3 | 等待 ~1s | DUT 进入 Normal，开始发送 Ring 消息 | PCAN 捕获到 DUT 发出的 Ring 消息 (Data[1]=0x0A) |

---

## 用例 3: LimpHome 进入与恢复

**前置条件**: DUT 处于 Normal 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 3.1 | DUT 在 Normal，PCAN 停止发送任何消息 | DUT TMax 超时，进入 LimpHome | UART: `NM [ch0] State: NORMAL_OP -> LIMPHOME`（约 2s 后）。PCAN 捕获到 LimpHome 消息 (Data[0]=0x04) |
| 3.2 | PCAN 发送 Ring 消息 (CAN ID 0x402, Data[0]=0x02, Data[1]=0x0B) | DUT 检测到网络恢复，退出 LimpHome | UART: `NM [ch0] State: LIMPHOME -> INITRESET` → `NM [ch0] State: INITRESET -> NORMAL_OP` |

---

## 用例 4: 通信禁用/启用

**前置条件**: DUT 处于 Normal 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 4.1 | DUT 在 Normal，周期性发送 Ring | PCAN 持续收到 Ring 消息 | 间隔 ~1s |
| 4.2 | 调用 `Nm_DisableCommunication(0)` | DUT 停止发送 NM PDU | PCAN 静默 ≥ 3s。UART 日志中 NM 状态未变化（保持 Normal） |
| 4.3 | 调用 `Nm_EnableCommunication(0)` | DUT 恢复发送 Ring 消息 | PCAN 恢复收到 Ring 消息 |

---

## 用例 5: 用户数据收发

**前置条件**: DUT 处于 Normal 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 5.1 | 调用 `Nm_SetUserData(0, {0xAA,0xBB}, 2)` | 用户数据已缓存 | API 返回 NM_E_OK |
| 5.2 | 等待下一条 DUT 发出的 Ring 消息 | Ring 消息携带用户数据 | PCAN 捕获: Data[2]=0xAA, Data[3]=0xBB (OpCode 模式 PDU 布局: Byte0=OpCode, Byte1=NodeId, Byte2..7=UserData) |
| 5.3 | PCAN 发送 Ring 消息 (Data[2..3]=0xCC,0xDD) | DUT 收到用户数据 | 调用 `Nm_GetUserData(0, rxBuf, &nodeId)` 返回 data={0xCC,0xDD}, nodeId=发送者 ID |

---

## 用例 6: 定时器精度验证

**前置条件**: DUT 处于 Normal 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 6.1 | 连续记录 10 条 Ring 消息的 PCAN 时间戳 | 消息间隔为 TTyp ± 200ms | 计算相邻消息时间差，均值 ≈ 1000ms，无明显漂移 |
| 6.2 | 停止所有外部消息，记录 DUT 从最后一条 Ring 到第一条 LimpHome 的时间 | 间隔 ≈ TMax (2000ms) | 误差 < 300ms |

---

## 用例 7: CAN Bus-Off 恢复

**前置条件**: DUT 处于 Normal 状态。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 7.1 | 调用 `Nm_ControllerBusOff(0)` | DUT 进入 LimpHome | UART: `NM [ch0] State: NORMAL_OP -> LIMPHOME` |
| 7.2 | PCAN 发送 Ring 消息 | DUT 检测到网络恢复，退出 LimpHome | UART: `NM [ch0] State: LIMPHOME -> INITRESET` → `NM [ch0] State: INITRESET -> NORMAL_OP` |

---

## 用例 8: 多节点逻辑环验证（可选）

> ⚠️ **需要两个物理 DUT**，单板环境暂不执行。

**前置条件**: 两个 DUT 均配置 Direct NM，不同 Node ID (0x0A, 0x0B)。

| 步骤 | 操作 | 预期结果 | 判定标准 |
|------|------|----------|----------|
| 8.1 | DUT_A (NodeID=0x0A) NetworkRequest | DUT_A 发送 Alive → Ring | PCAN 捕获 |
| 8.2 | DUT_B (NodeID=0x0B) NetworkRequest | DUT_B 发送 Alive 后，收到 DUT_A 的 Ring | DUT_B 按 NodeID 顺序加入逻辑环 |
| 8.3 | DUT_A NetworkRelease | DUT_A 进入 Prepare Bus-Sleep，发送 sleep.ind | DUT_B 收到 sleep.ind，DUT_A 最终进入 Bus-Sleep |
| 8.4 | DUT_B 继续正常通信 | DUT_B 不受 DUT_A 睡眠影响 | DUT_B 保持 Normal，Ring 消息正常发送 |
