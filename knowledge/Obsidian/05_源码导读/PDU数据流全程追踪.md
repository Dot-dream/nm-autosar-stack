---
tags:
  - architecture
  - pdu-dataflow
  - sequence-diagram
---

# PDU 数据流全程追踪

> NM PDU (8 字节网络管理消息) 的完整生命周期 — 从状态机组包发起到 CAN 总线上行、下行接收、到应用层读取。

---

## 1. PDU 字节布局

| 偏移 | 字段 | 大小 | 说明 |
|------|------|------|------|
| `[0]` | OpCode | 1 byte | 消息类型位图 (Alive=0x01, Ring=0x02, LimpHome=0x04, Active=0x40 等) |
| `[1]` | NodeID | 1 byte | 源节点 ID (0..31) |
| `[2..7]` | UserData | 0..6 bytes | 应用自定义数据 |

---

## 2. 发送端完整序列

```mermaid
sequenceDiagram
    participant FSM as Direct_FSM<br/>(状态机)
    participant Send as Direct_SendPdu<br/>(组包)
    participant CTX_D as g_channels[].txUserData<br/>(Direct 内部缓冲区)
    participant CTX_N as ctx.txUserData<br/>(Nm Core 缓冲区)
    participant Transmit as CanNm_Transmit<br/>(平台弱符号)
    participant CAN as CAN Driver/Hardware<br/>(CAN 控制器)
    participant BUS as CAN Bus<br/>(物理总线)

    Note over FSM: 状态: NORMAL<br/>TTyp 定时器到期
    FSM->>FSM: Nm_Timer_IsExpired(hTTyp) == TRUE
    FSM->>FSM: Nm_Timer_Start(hTTyp)
    FSM->>Send: Direct_SendPdu(ctx)

    Note over Send: 构建 8 字节 PDU

    Send->>Send: pdu[0..7] = 0x00
    alt wireFormat == CAN_ID_MODE
        Send->>Send: pdu[0] = pduOpCodeRing (0x02)
    else wireFormat == OPCODE_MODE
        Send->>Send: pdu[0] = NM_OP_RING_BIT (0x02)
        Send->>Send: pdu[0] |= NM_OP_ACTIVE_BIT (0x40)
        opt repeatMsgRequested
            Send->>Send: pdu[0] |= NM_OP_REPEAT_MSG_BIT (0x20)
        end
    end

    Send->>Send: pdu[1] = ctx->config->nodeId
    loop i: 0..txUserDataLen-1
        CTX_D->>Send: pdu[i+2] = txUserData[i]
    end

    Send->>Send: txPending = 1
    Send->>Send: txCounter++

    Send->>Transmit: CanNm_Transmit(channel, pdu, 8)
    Note over Transmit: 弱符号, 需平台实现覆盖

    Transmit->>CAN: CanIf_Transmit / CAN_Write
    Note over CAN: 写入 CAN 邮箱 / FIFO
    CAN-->>Transmit: CANNM_E_OK (入队成功)

    Note over CAN,BUS: CAN 控制器自动仲裁并发送
    CAN->>BUS: NM PDU on bus<br/>ID = base + nodeId (wireFormat CAN_ID)<br/>or singleCanId (wireFormat OPCODE)

    Note over Send: 注意: Direct_SendPdu 忽略<br/>CanNm_Transmit 返回值
```

### 2.1 发送端关键数据缓冲区

| 缓冲区 | 位置 | 数据类型 | 写入者 | 操作时机 |
|--------|------|----------|--------|----------|
| `ctx->txUserData[]` | Nm Core `channels[]` | uint8[6] | `Nm_SetUserData` | 应用层调用 |
| `g_channels[].txUserData[]` | Direct 内部 `CanNmOsekDirect_ChannelType` | uint8[6] | `CanNmOsekDirect_SetUserData` | Nm_SetUserData 转发, Init |
| `pdu[8]` | 栈变量 `Direct_SendPdu` | uint8[8] | `Direct_SendPdu` 现场构建 | FSM 每次发送前 |
| 用户数据传递链路: | `Nm_SetUserData` | -> `ctx->txUserData[]` | -> `CanNm_SetUserData(vtable)` | -> `g_channels[].txUserData[]` |
> 注意: `Direct_SendPdu` **直接从 `ctx->config->nodeId` 和 `ctx->txUserData[]` 读取**, 不经过 Nm Core 的 `ctx` 中的 `txUserData`。Nm Core 的 `txUserData` 仅作为 Nm Core 层的缓存备份。

---

## 3. 接收端完整序列

```mermaid
sequenceDiagram
    participant BUS as CAN Bus
    participant CAN as CAN Driver/ISR
    participant NRX as Nm_RxIndication<br/>(Nm Core)
    participant CTX_N as ctx.lastRxPdu<br/>(Nm Core PDU 缓存)
    participant CRX as CanNm_RxIndication<br/>(vtable dispatch)
    participant PRX as Direct_ProcessRx<br/>(FSM 接收处理)
    participant CTX_D as g_channels[].lastRxPdu<br/>(Direct 内部 PDU 缓存)
    participant FSM as Direct_FSM<br/>(状态机)
    participant APP as Application<br/>(应用层)
    participant Tmr as Nm_Timer<br/>(定时器)

    Note over BUS: NM PDU on bus

    CAN->>CAN: CAN RX interrupt
    CAN->>NRX: Nm_RxIndication(channel, data, length)

    Note over NRX: Step 1: Nm Core 缓存
    NRX->>NRX: 参数校验 (data!=NULL, 1<=len<=8)
    NRX->>CTX_N: ctx->lastRxPdu.data[] = data[]
    NRX->>CTX_N: ctx->lastRxPdu.length = len
    NRX->>CTX_N: ctx->lastRxNodeId = data[1]
    NRX->>CTX_N: ctx->rxPduAvailable = 1

    Note over NRX: Step 2: 转发到 CanNm
    NRX->>CRX: CanNm_RxIndication(ch, data, len)

    Note over CRX: CanNm.c: CanNm_GetCtxChecked
    CRX->>PRX: vtable->RxIndication(ch, data, len)

    Note over PRX: Direct_ProcessRx
    PRX->>PRX: 参数校验 (data!=NULL, len>=2)
    PRX->>CTX_D: ctx->lastRxPdu.data[] = data[]
    PRX->>CTX_D: ctx->lastRxPdu.length = len
    PRX->>CTX_D: ctx->lastRxNodeId = data[1]
    PRX->>CTX_D: ctx->rxPduAvailable = 1
    PRX->>PRX: ctx->rxCounter++

    Note over PRX,Tmr: 复位 TMax 接收看门狗
    PRX->>Tmr: Nm_Timer_Start(hTMax)
    Tmr-->>PRX: TMax 复位, 开始新周期

    alt 状态 == BUSSLEEP (被动唤醒)
        PRX->>FSM: Direct_ChangeState(CANNM_STATE_INITRESET)
        PRX->>APP: Nm_Core_DispatchNetworkStart(ch)
        Note over APP: 触发 Nm_NetworkStartIndication 回调
    end

    Note over PRX: 可选的 PDU Rx 回调
    opt pduRxCallback != NULL
        PRX->>APP: config->pduRxCallback(ch, &lastRxPdu)
    end

    Note over APP: 应用层异步获取 PDU 数据
    APP->>CTX_N: Nm_GetPduData(ch, buffer)
    CTX_N-->>APP: buffer[] = lastRxPdu.data[0..len]
    APP->>CTX_N: Nm_GetUserData(ch, buffer, &nodeId)
    CTX_N-->>APP: buffer[] = data[2..len], nodeId
```

### 3.1 接收端关键检查点

| 检查点 | 位置 | 校验内容 |
|--------|------|----------|
| 初始化检查 | Nm_RxIndication:552 | `Nm_Core.initialized == 0` 则丢弃 |
| 参数检查 | Nm_RxIndication:555 | `NULL == data \|\| 0 == len \|\| len > 8` 则丢弃 |
| 通道检查 | Nm_RxIndication:560 | `ctx == NULL` (无效 handle) 丢弃 |
| PDU 长度检查 | Direct_ProcessRx:146 | `len < 2` (无 NodeID) 丢弃 |
| vtable 检查 | CanNm_GetCtxChecked:203 | `ctx == NULL \|\| canNmVtable == NULL` 丢弃 |

---

## 4. 发送确认 (TxConfirmation) 序列

```mermaid
sequenceDiagram
    participant CAN as CAN Driver TX Complete ISR
    participant NTX as Nm_TxConfirmation<br/>(Nm Core)
    participant CTX as CanNm_TxConfirmation<br/>(vtable dispatch)
    participant DC as Direct TxConfirmation

    CAN->>NTX: Nm_TxConfirmation(channel)
    NTX->>NTX: 检查 initialized
    NTX->>CTX: CanNm_TxConfirmation(ch)

    CTX->>DC: vtable->TxConfirmation(ch)
    DC->>DC: ctx->txPending = 0
    Note over DC: Direct: 清除发送等待标志<br/>Indirect: 空函数 (不发送)<br/>AUTOSAR: 递减 txCnt
```

---

## 5. Bus-Off 事件序列

```mermaid
sequenceDiagram
    participant CAN as CAN Controller (Bus-Off detect)
    participant NCB as Nm_ControllerBusOff<br/>(Nm Core)
    participant CCB as CanNm_ControllerBusOff<br/>(vtable dispatch)
    participant DC as Direct_ControllerBusOff

    CAN->>NCB: Nm_ControllerBusOff(ch)
    NCB->>NCB: 校验通道, 设置 ctx->busOffActive = 1
    NCB->>CCB: CanNm_ControllerBusOff(ch)

    CCB->>DC: vtable->ControllerBusOff(ch)
    DC->>DC: Direct_ChangeState(CANNM_STATE_LIMPHOME)
    Note over DC: 状态切换触发 DispatchStateChange<br/>进入 LimpHome 模式
    DC->>DC: Nm_Timer_Start(hTError)
    Note over DC: 启动 LimpHome 消息发送周期<br/>之后 FSM 按 TError 间隔发送 LimpHome PDU
```

---

## 6. 数据缓冲区全景

| 缓冲区 | 所属结构 | 大小 | 写入者 | 读取者 | 生命周期 |
|--------|----------|------|--------|--------|----------|
| `ctx->lastRxPdu` | Nm_ChannelContextType | 8B + 1B len | `Nm_RxIndication` | `Nm_GetPduData`, `Nm_GetUserData` | 每次 Rx 覆盖 |
| `ctx->txUserData[]` | Nm_ChannelContextType | 6B | `Nm_SetUserData` | (Nm Core 层备份, 不做实际发送) | 应用设置 |
| `g_channels[].lastRxPdu` | CanNmOsekDirect_ChannelType | 8B + 1B len | `Direct_ProcessRx` | CanNm 内部状态机 | 每次 Rx 覆盖 |
| `g_channels[].txUserData[]` | CanNmOsekDirect_ChannelType | 6B | `CanNmOsekDirect_SetUserData` 转发 | `Direct_SendPdu` | 应用设置 |
| `pdu[8]` (栈) | `Direct_SendPdu` 函数 | 8B | 每次 `Direct_SendPdu` 构造 | `CanNm_Transmit` | 函数调用期间 |
| (CAN 驱动) | 平台 CAN 驱动发送邮箱/FIFO | 8B | `CanNm_Transmit` | CAN 控制器 | 发送完成后释放 |

**数据复制次数 (发送路径)**:
1. `txUserData[6]` (ROM/应用) -> `g_channels[].txUserData[6]` (Nm_SetUserData 时)
2. `g_channels[].txUserData[]` -> `pdu[2..7]` (Direct_SendPdu 组包时)
3. `pdu[8]` -> CAN 邮箱 (CanNm_Transmit)

**数据复制次数 (接收路径)**:
1. CAN 硬件 -> ISR 数据指针
2. ISR 数据 -> `ctx->lastRxPdu` (Nm Core 缓存, Nm_RxIndication)
3. ISR 数据 -> `g_channels[].lastRxPdu` (CanNm 内部缓存, Direct_ProcessRx)
4. `ctx->lastRxPdu` -> 应用缓冲区 (Nm_GetPduData)

---

## 7. 相关文件

- [[Nm_Core源码导读]] — Nm_RxIndication 和 Nm_TxConfirmation 实现
- [[CanNm适配层源码导读]] — CanNm_RxIndication vtable 分发
- [[数据结构运行时全景]] — Nm_PduType 及缓冲区结构
- [[函数调用关系总图]] — PDU 收发在全局调用图中的位置
