# NM 自动化测试 — CAN 通讯协议

> 版本: V1.0 | 日期: 2026-05-29
> 适用范围: USB2CAN (UTA0503) ↔ YTM32B1MC0 Demo DUT

---

## 1. 物理层

| 参数     | 值                 |
| -------- | ------------------ |
| 波特率   | 500 kbps           |
| 帧格式   | 标准帧 (11-bit ID) |
| 终端电阻 | 120Ω (双方)       |

---

## 2. CAN ID 分配

|  CAN ID  |   方向   | 用途                                    | 协议类型         |
| :-------: | :-------: | --------------------------------------- | ---------------- |
| `0x402` | DUT → PC | NM PDU 发送 (OSEK Direct / AUTOSAR)     | 见 §3.1 / §3.3 |
| `0x402` | PC → DUT | 模拟远程 NM PDU (OSEK Direct / AUTOSAR) | 见 §3.1 / §3.3 |
| `0x403` | PC → DUT | 模拟远程 NM PDU (备用通道)              | 见 §3.1         |
| `0x41C` | PC → DUT | 模拟应用 CAN 消息 (OSEK Indirect 用)    | 见 §3.2         |
| `0x7E0` | PC → DUT | **NM 控制命令** (测试协议核心)    | 见 §4           |

---

## 3. NM PDU 格式

### 3.1 OSEK Direct NM PDU

CAN ID: `0x402`
DLC: `8`

| Byte | 字段                   | 说明                              |
| :--: | ---------------------- | --------------------------------- |
|  0  | OpCode                 | 消息类型 (见下表)                 |
|  1  | DestNodeId / SrcNodeId | 目标节点 ID (TX) / 源节点 ID (RX) |
| 2-7 | UserData               | 用户数据 (6 bytes)                |

**OpCode 定义**:

|  OpCode  | 名称                       | 说明                        |
| :------: | -------------------------- | --------------------------- |
| `0x01` | Alive                      | 节点上线声明                |
| `0x02` | Ring                       | 逻辑环令牌传递              |
| `0x04` | LimpHome                   | 降级模式                    |
| `0x11` | Alive + SleepInd           | Alive 消息带 Sleep 指示     |
| `0x12` | Ring + SleepInd            | Ring 消息带 Sleep 指示      |
| `0x14` | LimpHome + SleepInd        | LimpHome 消息带 Sleep 指示  |
| `0x22` | Ring + SleepAck            | Ring 消息带 Sleep 确认      |
| `0x32` | Ring + SleepInd + SleepAck | Ring 消息带 Sleep 指示+确认 |

### 3.2 OSEK Indirect NM (应用消息)

CAN ID: `0x41C` (示例，可配置)
DLC: `8`
格式: 任意，间接 NM 仅判断消息是否到达，不解析内容。

### 3.3 AUTOSAR NM PDU

CAN ID: `0x402`
DLC: `8`

| Byte | 字段         | 说明                        |
| :--: | ------------ | --------------------------- |
|  0  | SourceNodeId | 源节点 ID                   |
|  1  | CBV          | Control Bit Vector (见下表) |
| 2-7 | UserData     | 用户数据 (6 bytes)          |

**CBV (Control Bit Vector) 位定义**:

| Bit | 名称                 | 值=1 含义                      | 值=0 含义             |
| :-: | -------------------- | ------------------------------ | --------------------- |
|  0  | RepeatMessageRequest | 节点正在唤醒，请求网络保持活跃 | 正常操作              |
|  1  | ActiveWakeup         | 主动请求网络 (应用层需要通信)  | 无通信需求 / 准备休眠 |
|  2  | PNI                  | Partial Network Info 存在      | 无 PN 信息            |
|  3  | CoordinatorSleep     | 协调器指示可休眠               | —                    |
| 4-5 | UserDataLen          | 用户数据长度 (0-6 bytes)       | —                    |
| 6-7 | Reserved             | 保留                           | —                    |

**状态 ↔ CBV 对应关系**:

| DUT 状态             | Bit0 | Bit1 | Bit3 |
| -------------------- | :--: | :--: | :--: |
| BUS_SLEEP            |  —  |  —  |  —  |
| REPEAT_MESSAGE       |  1  |  1  |  0  |
| NORMAL_OPERATION     |  0  |  1  |  0  |
| READY_SLEEP          |  0  |  0  |  0  |
| SYNCHRONIZE (协调器) |  0  |  0  |  1  |
| PREPARE_BUS_SLEEP    |  0  |  0  |  0  |

---

## 4. NM 控制命令帧格式

CAN ID: `0x7E0`
DLC: `1`
方向: **PC → DUT**

| Data[0] | 命令名              | 对应 DUT API                   | 说明                        |
| :------: | ------------------- | ------------------------------ | --------------------------- |
| `0x01` | `NETWORK_REQUEST` | `Nm_NetworkRequest(0)`       | 请求进入网络模式            |
| `0x02` | `NETWORK_RELEASE` | `Nm_NetworkRelease(0)`       | 释放网络，准备休眠          |
| `0x03` | `DISABLE_COMM`    | `Nm_DisableCommunication(0)` | 禁止 NM PDU 发送            |
| `0x04` | `ENABLE_COMM`     | `Nm_EnableCommunication(0)`  | 恢复 NM PDU 发送            |
| `0x05` | `BUSOFF`          | `Nm_ControllerBusOff(0)`     | 模拟 CAN Bus-Off            |
| `0x06` | `IG_ON`           | `Nm_NetworkRequest(0)`       | **模拟点火 ON 事件**  |
| `0x07` | `IG_OFF`          | `Nm_NetworkRelease(0)`       | **模拟点火 OFF 事件** |

> 注: `IG_ON` 和 `IG_OFF` 在 DUT 侧行为与 `NETWORK_REQUEST` / `NETWORK_RELEASE` 相同，区别在于测试用例语义——`IG_ON/OFF` 代表整车电源事件，`NETWORK_REQUEST/RELEASE` 代表通信层主动控制。

---

## 5. 测试用例 ↔ CAN 帧映射

### 5.1 OSEK Direct NM

| 用例                  | 步骤                          | CAN 帧                                  |
| --------------------- | ----------------------------- | --------------------------------------- |
| D01 基本生命周期      | DUT 上电自动 NetworkRequest   | 等待 Alive → Ring                      |
|                       | PC 发送 NetworkRelease        | `0x7E0 [0x02]`                        |
|                       | 确认 Bus-Sleep                | 静默                                    |
| D02 被动唤醒          | PC 发送 Alive (模拟远程节点)  | `0x402 [0x01, 0x0B, ...]`             |
|                       | 确认 DUT 恢复 Ring            | 等待 `0x402 [0x02, ...]`              |
| D03 LimpHome          | PC 停止发送 → TMax 超时      | 等待 `0x402 [0x04, ...]`              |
|                       | PC 发送 Ring (恢复)           | `0x402 [0x02, 0x0B, ...]`             |
| D04 通信控制          | PC 发送 Disable               | `0x7E0 [0x03]`                        |
|                       | PC 发送 Enable                | `0x7E0 [0x04]`                        |
| D05 UserData          | PC 发送 Ring + UserData       | `0x402 [0x02, 0x0B, 0xCC, 0xDD, ...]` |
| D06 定时器精度        | 自然收发, 记录时间戳          | 等待 10 条 Ring                         |
| D07 BusOff            | PC 发送 BusOff                | `0x7E0 [0x05]`                        |
|                       | PC 发送 Ring (恢复)           | `0x402 [0x02, 0x0B, ...]`             |
| **D08 IG 周期** | PC 发送 IG_ON                 | `0x7E0 [0x06]`                        |
|                       | 确认 Alive → Ring → Normal  | 等待                                    |
|                       | PC 发送 IG_OFF                | `0x7E0 [0x07]`                        |
|                       | 确认 PrepareSleep → BusSleep | 静默                                    |

### 5.2 OSEK Indirect NM

| 用例             | 步骤                   | CAN 帧                      |
| ---------------- | ---------------------- | --------------------------- |
| I01 基本生命周期 | PC 发送应用消息        | `0x41C [0x10, 0x01, ...]` |
|                  | 停止发送 → 超时       | —                          |
| I02 唤醒         | PC 发送应用消息        | `0x41C [0x10, 0x01, ...]` |
| I03 消息间歇     | PC 按间隔发送          | `0x41C [...]`             |
| I04 主动控制     | PC 发送 NetworkRequest | `0x7E0 [0x01]`            |

### 5.3 AUTOSAR NM

| 用例                  | 步骤                             | CAN 帧                               |
| --------------------- | -------------------------------- | ------------------------------------ |
| A01 基本生命周期      | DUT 自动 NetworkRequest          | 等待 CBV 0x03                        |
|                       | PC 发送 NetworkRelease           | `0x7E0 [0x02]`                     |
|                       | 确认 CBV Bit1=0                  | 等待 CBV 0x00                        |
| A02 CBV 编码          | 自然收发, 验证 CBV 位            | 等待 NM PDU                          |
| A03 网络唤醒          | PC 发送 NM PDU (CBV=0x03)        | `0x402 [0x0B, 0x03, ...]`          |
| A04 睡眠中止          | NetworkRelease → NetworkRequest | `0x7E0 [0x02]` → `0x7E0 [0x01]` |
| A05 远程拉回          | PC 发送 NM PDU (Bit0=1+Bit1=1)   | `0x402 [0x0B, 0x03, ...]`          |
| A06 PDU 格式          | 验证 DLC=8, SourceNodeId, CAN ID | 等待 5 条 NM PDU                     |
| **A07 IG 周期** | PC 发送 IG_ON                    | `0x7E0 [0x06]`                     |
|                       | 确认 REPEAT_MESSAGE CBV 0x03     | 等待                                 |
|                       | PC 发送 IG_OFF                   | `0x7E0 [0x07]`                     |
|                       | 确认 READY_SLEEP CBV Bit1=0      | 等待 CBV 0x00                        |

---

## 6. 时序要求

| 场景                                    | 参数                     | 预期值  |   容差   |
| --------------------------------------- | ------------------------ | ------- | :-------: |
| OSEK Direct: Alive → Ring 间隔         | TTyp                     | 1000 ms | ±200 ms |
| OSEK Direct: 最后 Ring → LimpHome      | TMax                     | 2000 ms | ±300 ms |
| OSEK Direct: PrepareSleep → BusSleep   | TWbs                     | 2000 ms | ±500 ms |
| OSEK Indirect: 最后消息 → WaitBusSleep | TMax                     | 2000 ms | ±300 ms |
| AUTOSAR: NM PDU 周期间隔                | timerTyp                 | 1000 ms | ±200 ms |
| AUTOSAR: Repeat Message 持续时间        | txCountLimit × timerTyp | 8000 ms | ±2000 ms |

---

## 7. 测试拓扑

```
┌─────────────┐                    ┌──────────────────┐
│   PC (Win)  │ ◄── USB ──► │ USB2CAN (UTA0503) │
│ test_runner │                    │  CAN1  CAN2      │
└─────────────┘                    └──┬───────┬───────┘
                                     │       │
                              CAN_H ─┤       │
                              CAN_L ─┘       │
                                     ┌───────┘
                                     │
                            ┌────────┴────────┐
                            │  YTM32B1MC0     │
                            │  (Demo_Project) │
                            │  CAN: PB8/PB9   │
                            └─────────────────┘
```

- 单通道连接 (CAN1)，DUT 和 PC 在同一 CAN 总线上
- 两端均配置 120Ω 终端电阻
- PC 端模拟远程节点 (NodeID=0x0B)
- DUT 本地节点 (NodeID=0x0A，可配置)
