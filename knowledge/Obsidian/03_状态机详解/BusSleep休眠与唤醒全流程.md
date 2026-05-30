# Bus-Sleep 休眠与唤醒全流程

> 属于 [[../00_MOC_总索引|MOC 总索引]] > **03_状态机详解**

Bus-Sleep 是 NM 模块的核心目标：协调所有节点安全地进入低功耗模式。
唤醒过程则确保在需要通信时快速恢复网络。

---

## 完整生命周期流程图

```mermaid
stateDiagram-v2
    [*] --> Bus_Sleep : 系统启动 / Nm_Init()<br/>(初始状态)

    state Bus_Sleep {
        [*] --> 空闲等待
        空闲等待 --> 被动唤醒 : 收到 NM PDU
        空闲等待 --> 主动唤醒 : Nm_NetworkRequest()
    }

    state Network_Mode {
        被动唤醒 --> NORMAL
        主动唤醒 --> NORMAL
    }

    Bus_Sleep --> Network_Mode : 唤醒

    Network_Mode --> Prepare_Sleep : Nm_NetworkRelease()

    state Prepare_Sleep {
        [*] --> TWbs等待
        TWbs等待 --> Bus_Sleep : TWbs 超时
        TWbs等待 --> 继续发送 : 未超时
    }

    Prepare_Sleep --> Bus_Sleep

    note right of Bus_Sleep
        控制器状态: CAN 控制器关闭 (低功耗)
        回调: Nm_BusSleepMode
    end note

    note right of Network_Mode
        控制器状态: CAN 控制器开启 (正常)
        回调: Nm_NetworkMode
    end note
```

---

## 主动唤醒流程 (NetworkRequest)

```mermaid
sequenceDiagram
    participant App as 应用层
    participant Nm as Nm Core
    participant FSM as OSEK Direct FSM
    participant Timer as Nm_Timer
    participant Can as CAN 控制器

    Note over App,Can: === Bus-Sleep 状态 ===
    Can->>Can: CAN 控制器关闭

    App->>Nm: Nm_NetworkRequest(channel)
    Nm->>FSM: Direct_ChangeState(INIT)

    FSM->>Can: CanNm_ControllerEnable()<br/>(打开 CAN 控制器)

    FSM->>FSM: Direct_ChangeState(INITRESET)
    FSM->>Timer: Nm_Timer_Start(hTTyp)
    FSM->>Can: Alive PDU 发送

    Note over App,Can: === TTyp 到期 (1000ms) ===
    FSM->>FSM: Direct_ChangeState(NORMAL)
    FSM->>Nm: Nm_Core_DispatchNetworkMode()<br/>→ Nm_NetworkMode()

    Note over App,Can: === 网络模式 ===
    App->>App: 允许应用层通信
```

---

## 被动唤醒流程 (收到 NM PDU)

```mermaid
sequenceDiagram
    participant Can as CAN 总线
    participant Nm as Nm Core
    participant FSM as OSEK Direct FSM
    participant App as 应用层

    Note over App,Can: === Bus-Sleep 状态 ===

    Can->>Nm: Nm_RxIndication(ch, PDU)<br/>(CAN 控制器唤醒中断)
    Nm->>FSM: Direct_ProcessRx(data, len)

    FSM->>FSM: if (state == BUSSLEEP)<br/>→ ChangeState(INITRESET)
    FSM->>Nm: Nm_Core_DispatchNetworkStart()<br/>→ Nm_NetworkStartIndication()

    App->>App: 处理唤醒事件<br/>(启动通信任务等)

    Note over FSM: TTyp 到期 → NORMAL<br/>→ Nm_NetworkMode()
```

---

## 主动休眠流程 (NetworkRelease)

```mermaid
sequenceDiagram
    participant App as 应用层
    participant Nm as Nm Core
    participant FSM as OSEK Direct FSM
    participant Timer as Nm_Timer
    participant Can as CAN 控制器

    Note over App,Can: === NORMAL 状态 (网络模式) ===

    App->>Nm: Nm_NetworkRelease(channel)
    Nm->>FSM: Direct_ChangeState(NORMALPREPSLEEP)

    FSM->>Timer: Nm_Timer_Start(hTWbs)

    Note over FSM: 继续发送 Ring PDU<br/>(每 TTyp 周期)

    Note over App,Can: === TWbs 到期 (2000ms) ===
    Timer-->>FSM: Nm_Timer_IsExpired(hTWbs) = TRUE

    FSM->>FSM: Direct_ChangeState(BUSSLEEP)
    FSM->>Can: CanNm_ControllerDisable()<br/>(关闭 CAN 控制器)

    FSM->>Nm: Nm_Core_DispatchBusSleep()<br/>→ Nm_BusSleepMode()

    App->>App: 进入低功耗模式
```

---

## 三种 NM 模式的休眠/唤醒差异

| 步骤 | OSEK Direct | OSEK Indirect | AUTOSAR NM |
|------|-------------|---------------|------------|
| **初始状态** | BUSSLEEP | OFF | BUS_SLEEP |
| **主动唤醒入口** | NetworkRequest → INIT | NetworkRequest → AWAKE | NetworkRequest → REPEAT_MESSAGE |
| **被动唤醒入口** | 收到 NM PDU → INITRESET | 收到应用消息 → AWAKE | 收到 NM PDU → REPEAT_MESSAGE |
| **网络模式** | NORMAL (Ring 循环) | NORMAL (监听) | NORMAL_OPERATION (广播) |
| **休眠触发** | NetworkRelease → NORMALPREPSLEEP | NetworkRelease → WAITBUSSLEEP | NetworkRelease → READY_SLEEP 或 SYNCHRONIZE |
| **休眠等待** | TWbs (继续发 Ring) | TWbs (继续监听) | hNmWaitBusSleep (继续广播) |
| **最终休眠** | TWbs → BUSSLEEP | TWbs → BUSSLEEP | TWbs → BUS_SLEEP |
| **回调** | Nm_NetworkMode / Nm_BusSleepMode 等 | 同 Direct | Nm_NetworkStartIndication / NetworkMode / BusSleepMode |

---

## 回调触发顺序 (完整一次休眠→唤醒)

```mermaid
sequenceDiagram
    participant App as 应用层回调

    Note over App: === 休眠过程 ===
    App->>App: 1. Nm_StateChangeNotification<br/>(NORMAL_OPERATION → PREPARE_BUS_SLEEP)
    App->>App: 2. Nm_PrepareBusSleepMode(ch)

    Note over App: === TWbs 到期 → 休眠 ===
    App->>App: 3. Nm_StateChangeNotification<br/>(PREPARE_BUS_SLEEP → BUS_SLEEP)
    App->>App: 4. Nm_BusSleepMode(ch)

    Note over App: === 被动唤醒 ===
    App->>App: 5. Nm_NetworkStartIndication(ch)
    App->>App: 6. Nm_StateChangeNotification<br/>(BUS_SLEEP → INITRESET)

    Note over App: === TTyp → 网络模式 ===
    App->>App: 7. Nm_StateChangeNotification<br/>(INITRESET → NORMAL_OPERATION)
    App->>App: 8. Nm_NetworkMode(ch)
```

应用层按此顺序接收回调，可以根据模式切换执行相应的系统级操作。

---

## 关键设计要点

1. **休眠前必须停止通信**: `Nm_PrepareBusSleepMode` 回调时，ComM 应阻止正常通信
2. **TWbs 期间继续发送**: 让其他节点知道本节点即将休眠
3. **CanNm_ControllerDisable**: 进入 Bus-Sleep 后关闭 CAN 控制器以省电
4. **被动唤醒依赖 CAN 硬件唤醒**: CAN 控制器需配置为支持总线唤醒 (Wake-on-CAN)

---

> 下一步: 阅读 [[../04_API参考/Nm_Public_API_19个函数|Nm Public API 19 个函数]]
