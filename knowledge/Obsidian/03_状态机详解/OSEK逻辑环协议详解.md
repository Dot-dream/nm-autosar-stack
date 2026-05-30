# OSEK 逻辑环协议详解

> 属于 [[../00_MOC_总索引|MOC 总索引]] > **03_状态机详解**

OSEK Direct NM 的核心是**逻辑环 (Logical Ring)** 协议。
节点按照 NodeID 排序形成环状结构，通过 Alive → Ring → LimpHome 三种消息协调网络。

---

## 逻辑环结构

```mermaid
graph TD
    subgraph Ring["OSEK 逻辑环 (按 NodeID 升序)"]
        N1["节点 A<br/>NodeID=0x01"]
        N2["节点 B<br/>NodeID=0x05"]
        N3["节点 C<br/>NodeID=0x0A"]
        N4["节点 D<br/>NodeID=0x10"]
    end

    N1 -->|"Ring 消息<br/>后继"| N2
    N2 -->|"Ring 消息<br/>后继"| N3
    N3 -->|"Ring 消息<br/>后继"| N4
    N4 -->|"Ring 消息<br/>后继<br/>回到最小 NodeID"| N1
```

- 每个节点知道自己的**前任** (NodeID 比自己小的最大节点) 和**后继** (NodeID 比自己大的最小节点)
- Ring 消息沿着环传递，每经一个节点就复位该节点的 TMax
- 最大 NodeID 的节点发送 Ring 到最小 NodeID，完成闭环

---

## Alive 消息：节点加入网络

```mermaid
sequenceDiagram
    participant NewNode as 新节点 (NodeID=0x0A)
    participant Ring as 环中现有节点
    participant CAN as CAN 总线

    Note over NewNode: INIT 状态<br/>发送 Alive PDU
    NewNode->>CAN: Alive PDU (包含本节点 ID)

    Note over Ring: 收到 Alive → 复位 TMax
    Note over Ring: 识别新节点加入

    Note over NewNode: INITRESET 状态<br/>TTyp 到期
    NewNode->>NewNode: 转到 NORMAL
    Note over NewNode: 加入 Ring 循环
```

**Alive 消息作用**:
1. 宣告本节点存在
2. 通知环中其他节点更新前任/后继关系
3. 复位所有节点的 TMax

---

## Ring 消息：逻辑环循环

```mermaid
sequenceDiagram
    participant A as 节点 A (0x01)
    participant B as 节点 B (0x05)
    participant C as 节点 C (0x0A)
    participant D as 节点 D (0x10)
    participant CAN as CAN 总线

    Note over A,D: === TTyp 周期 (1000ms) ===

    A->>CAN: Ring PDU (目标: B)
    Note over B: 收到 Ring → 复位 TMax
    Note over B: 识别自己是接收者

    B->>CAN: Ring PDU (目标: C)
    Note over C: 收到 Ring → 复位 TMax

    C->>CAN: Ring PDU (目标: D)
    Note over D: 收到 Ring → 复位 TMax

    D->>CAN: Ring PDU (目标: A, 回到最小 NodeID)
    Note over A: 收到 Ring → 环完整回环

    Note over A,D: === 下一周期 (1000ms 后) ===
```

**Ring 消息作用**:
1. 复位接收者的 TMax (证明总线正常)
2. 维持逻辑环循环
3. 可携带 sleep.ind / sleep.ack 指示休眠意愿

---

## LimpHome 消息：降级通信

```mermaid
sequenceDiagram
    participant A as 节点 A
    participant CAN as CAN 总线
    participant OtherNodes as 其他节点

    Note over A: NORMAL 状态<br/>TMax = 2000ms 看门狗
    Note over A: 2000ms 内未收到任何消息<br/>→ TMax 超时 → LIMPHOME

    A->>CAN: LimpHome PDU (TError = 1000ms 周期)
    Note over OtherNodes: 识别到 A 的 LimpHome

    Note over A: 持续发送 LimpHome...

    OtherNodes->>CAN: 任意 NM PDU (复位 A 的 TMax)
    A->>A: !Nm_Timer_IsExpired(TMax)<br/>→ 恢复 INIT
    A->>A: → INITRESET → NORMAL

    Note over A: 回到正常 Ring 循环
```

**LimpHome 消息作用**:
- 当节点收不到任何消息时，降级到独立发送模式
- 持续发出"我还在线"信号
- 收到任何 NM PDU 后立即恢复

---

## 完整消息时序总结

```mermaid
graph TD
    subgraph Normal["正常状态消息流"]
        N_Alive["Alive: 节点加入<br/>(INIT → INITRESET)"]
        N_Ring["Ring: 逻辑环循环<br/>(NORMAL / TWBS_NORMAL)"]
        N_Sleep["Ring + SleepInd: 休眠预告<br/>(NORMALPREPSLEEP)"]
    end

    subgraph Error["异常状态消息流"]
        E_Limp["LimpHome: 降级通信<br/>(LIMPHOME)"]
        E_LimpSleep["LimpHome: 降级休眠<br/>(LIMPHOMEPREPSLEEP)"]
    end

    N_Alive --> N_Ring
    N_Ring -->|"TMax 超时"| E_Limp
    E_Limp -->|"TMax 恢复"| N_Alive
    N_Ring -->|"NetworkRelease"| N_Sleep
    N_Sleep -->|"TWbs 到期"| BusSleep["BUSSLEEP"]
    E_Limp -->|"NetworkRelease"| E_LimpSleep
    E_LimpSleep -->|"TWbs 到期"| BusSleep
```

---

## 定时器与消息的关系

| 定时器 | 关联的消息 | 说明 |
|--------|-----------|------|
| TTyp | Ring | 在 NORMAL 状态下，每 TTyp 周期发送一次 Ring |
| TMax | 所有 NM PDU | 每次收到任何 NM PDU 都复位 TMax |
| TError | LimpHome | 在 LIMPHOME 状态下，每 TError 周期发送一次 LimpHome |
| TWbs | Ring / LimpHome | 休眠前等待周期，期间继续发送消息 |
| TTx | 重发 | 发送失败时的重试间隔 |

---

## 消息格式 (以 OpCode 模式为例)

| 字节 | Alive | Ring | LimpHome |
|:---:|-------|------|----------|
| Byte 0 | `0x41` (Alive bit + Active bit) | `0x42` (Ring bit + Active bit) | `0x44` (LimpHome bit + Active bit) |
| Byte 1 | NodeID | NodeID | NodeID |
| Byte 2-7 | 用户数据 (可选) | 用户数据 (可选) | 用户数据 (可选) |

Active 位 (`0x40`) 始终设置，表示本节点处于活跃状态。

---

> 下一步: 阅读 [[../03_状态机详解/LimpHome降级与恢复|LimpHome 降级与恢复]]
