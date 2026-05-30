# test_nm_state 7 个用例详解

> 属于 [[../00_MOC_总索引|MOC 总索引]] > **06_测试与验证**

`test_nm_state.c` 包含 7 个 PC 端单元测试用例，覆盖 OSEK Direct / Indirect NM 的全部关键路径。所有用例通过 `Test_Reset()` 初始化隔离环境，通过 `Test_AdvanceTime()` 驱动时间推进。

---

## Test 1: 基本状态转换 (BUS_SLEEP → NORMAL → PREPSLEEP → BUSSLEEP)

| 项目 | 内容 |
|------|------|
| **测试目标** | 验证 OSEK Direct NM 的完整生命周期：初始化 → 网络请求 → 正常通信 → 释放 → 休眠 |
| **前置条件** | `Test_Reset()` 清零所有状态，`Nm_Init(&g_config)` 初始化 |
| **操作步骤** | 1. `Nm_GetState()` → 确认初始状态为 `NM_STATE_BUS_SLEEP`<br>2. `Nm_NetworkRequest(0)` → 发起网络请求<br>3. `Test_AdvanceTime(2000)` → 跳过 INITRESET （TTyp = 1000ms）<br>4. `Nm_GetState()` → 确认进入 `NM_STATE_NORMAL_OPERATION`，mode = `NM_MODE_NETWORK`<br>5. 断言 `g_txCount > 0`（已发送 Alive + Ring）<br>6. `Nm_NetworkRelease(0)` → 释放网络<br>7. 确认进入 `NM_STATE_PREPARE_BUS_SLEEP`<br>8. `Test_AdvanceTime(2500)` → 等待 TWbs = 2000ms<br>9. 确认进入 `NM_STATE_BUS_SLEEP` |
| **预期结果** | 状态顺序: `BUS_SLEEP → INITRESET → NORMAL_OPERATION → PREPARE_BUS_SLEEP → BUS_SLEEP` |
| **覆盖状态转换** | `BUS_SLEEP → INITRESET` (NetworkRequest)<br>`INITRESET → NORMAL_OPERATION` (TTyp 期满)<br>`NORMAL_OPERATION → PREPARE_BUS_SLEEP` (NetworkRelease)<br>`PREPARE_BUS_SLEEP → BUS_SLEEP` (TWbs 期满) |

---

## Test 2: TMax 超时进入 LimpHome (NORMAL → LIMPHOME)

| 项目 | 内容 |
|------|------|
| **测试目标** | 验证在 NORMAL_OPERATION 中超过 TMax 时间未收到消息时进入 LimpHome 降级模式 |
| **前置条件** | 已进入 `NM_STATE_NORMAL_OPERATION`（Init → NetworkRequest → AdvanceTime 2000ms） |
| **操作步骤** | 1. `Nm_Init` + `Nm_NetworkRequest` + `Test_AdvanceTime(2000)` 进入 NORMAL<br>2. `Test_AdvanceTime(3000)` → 超过 TMax = 2000ms 无消息<br>3. `Nm_GetState()` → 断言为 `NM_STATE_LIMPHOME` |
| **预期结果** | 状态: `NORMAL_OPERATION → LIMPHOME` |
| **覆盖状态转换** | `NORMAL_OPERATION → LIMPHOME` (TMax 超时, 无 Ring 消息接收) |

---

## Test 3: LimpHome 收到消息恢复 (LIMPHOME → INITRESET)

| 项目 | 内容 |
|------|------|
| **测试目标** | 验证 LimpHome 状态下收到 NM 消息后恢复到 INITRESET，重新加入逻辑环 |
| **前置条件** | 已进入 `NM_STATE_LIMPHOME`（重复 Test2 的前置 + 操作） |
| **操作步骤** | 1. 确认当前在 LimpHome<br>2. `Test_RxNmMessage(NM_OP_RING_BIT, 0x0B)` → 模拟收到 Ring 消息<br>3. `Nm_MainFunction()` → 驱动状态机处理<br>4. `Nm_GetState()` → 断言 `state == NM_STATE_INITRESET \|\| state == NM_STATE_NORMAL_OPERATION` |
| **预期结果** | `LIMPHOME → INITRESET`（然后可继续过渡到 NORMAL_OPERATION） |
| **覆盖状态转换** | `LIMPHOME → INITRESET` (收到合法 NM PDU) |

---

## Test 4: Bus-Sleep 中唤醒 (BUSSLEEP → INITRESET)

| 项目 | 内容 |
|------|------|
| **测试目标** | 验证节点在 Bus-Sleep 状态下收到 NM 消息后被唤醒 |
| **前置条件** | 完整生命周期完成 → Bus-Sleep：`Init → NetworkRequest → AdvanceTime(2000) → NetworkRelease → AdvanceTime(3000)` |
| **操作步骤** | 1. 确认当前在 `NM_STATE_BUS_SLEEP`<br>2. `Test_RxNmMessage(NM_OP_ALIVE_BIT, 0x0B)` → 模拟收到 Alive 消息<br>3. `Nm_MainFunction()`<br>4. 断言 `state != NM_STATE_BUS_SLEEP`（已唤醒）<br>5. 断言 `g_cbNetworkStartCount > 0`（回调触发） |
| **预期结果** | `BUSSLEEP → INITRESET` 或更高，回调 `Nm_NetworkStartIndication` 触发 |
| **覆盖状态转换** | `BUS_SLEEP → INITRESET` (被动唤醒) |

---

## Test 5: 通信禁止/使能 (停止发送 → 恢复发送)

| 项目 | 内容 |
|------|------|
| **测试目标** | 验证 `Nm_DisableCommunication` 停止发送 NM PDU，`Nm_EnableCommunication` 恢复 |
| **前置条件** | 已在 `NM_STATE_NORMAL_OPERATION` |
| **操作步骤** | 1. 进入 NORMAL_OPERATION<br>2. 记录当前 `txBefore = g_txCount`<br>3. `Nm_DisableCommunication(0)` → 禁止通信<br>4. `Test_AdvanceTime(2000)` → 等待一个 TTyp 周期<br>5. 断言 `g_txCount == txBefore`（无新发送）<br>6. `Nm_EnableCommunication(0)` → 使能通信<br>7. `Test_AdvanceTime(2000)`<br>8. 断言 `g_txCount > txBefore`（恢复发送） |
| **预期结果** | 禁用期间 `g_txCount` 不变，使能后 `g_txCount` 增长 |
| **覆盖功能** | `Nm_DisableCommunication` / `Nm_EnableCommunication` API |

---

## Test 6: 用户数据设置/获取

| 项目 | 内容 |
|------|------|
| **测试目标** | 验证 User Data 的设置和接收解析功能 |
| **前置条件** | `Nm_Init` 后 |
| **操作步骤** | 1. `Nm_SetUserData(0, {0xAA, 0xBB}, 2)` → 设置用户数据<br>2. 模拟接收 PDU: `{0x01, 0x0B, 0xCC, 0xDD, ...}` → `Nm_RxIndication`<br>3. `Nm_GetUserData(0, rxData, &nodeId)`<br>4. 断言 `rxData[0] == 0xCC`<br>5. 断言 `rxData[1] == 0xDD`<br>6. 断言 `nodeId == 0x0B` |
| **预期结果** | 提取的 User Data 和 NodeId 与发送内容一致 |
| **覆盖功能** | `Nm_SetUserData` / `Nm_GetUserData` API，PDU 中 UserData 字段解析 |

---

## Test 7: Indirect NM 应用消息监控

| 项目 | 内容 |
|------|------|
| **测试目标** | 验证 OSEK Indirect NM 通过应用消息判定总线活跃并进入休眠 |
| **前置条件** | `nmMode = NM_MODE_INDIRECT`，其他配置同 Direct |
| **操作步骤** | 1. `Test_Reset()` + `Nm_Init(&indCfg)`（indCfg 中 nmMode = INDIRECT）<br>2. `Nm_GetState()` → 确认起始状态<br>3. 模拟收到应用 PDU: `{0x10, 0x01, ...}` → `Nm_RxIndication`<br>4. `Nm_MainFunction()`<br>5. `Test_AdvanceTime(5000)` → 无消息超时（超过 TMax = 2000ms + TWbs）<br>6. 断言 `state == NM_STATE_BUS_SLEEP` |
| **预期结果** | Indirect 模式下收到应用消息 → 进入活跃态；超时无消息 → 回到 Bus-Sleep |
| **覆盖状态转换** | Indirect: `BUS_SLEEP → NORMAL` (应用消息触发)<br>`NORMAL → WAITBUSSLEEP → BUS_SLEEP` (超时休眠) |

**注意**：Indirect NM 不发送 NM PDU（`g_txCount` 始终为 0），完全通过监听应用层 CAN 消息判定总线状态。

---

## 测试架构

```
main()
├── Test1_BasicStateTransitions()
├── Test2_LimpHomeEntry()
├── Test3_LimpHomeRecovery()
├── Test4_WakeupFromBusSleep()
├── Test5_CommunicationDisable()
├── Test6_UserData()
└── Test7_IndirectNM()
```

所有测试共享同一个 `g_channel` / `g_config` 模板，Test7 通过局部拷贝修改 `nmMode` 字段。每项测试前调用 `Test_Reset()` 保证环境隔离。

---

> 测试框架的 shim 机制详解见 [[PC端测试框架详解|PC端测试框架详解]]。编译运行命令见 [[编译与运行命令|编译与运行命令]]。
