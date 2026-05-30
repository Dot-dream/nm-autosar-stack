# 常见问题 FAQ

> 属于 [[../00_MOC_总索引|MOC 总索引]] > **07_集成与附录**

NM 模块集成和开发中的常见编译、链接、架构问题及解决方案。

---

## Q1: 编译报 `#error "Nm_EnterCritical() must be defined"`

**错误信息**：
```
Nm_OsIf.h: fatal error: #error "Nm_EnterCritical() must be defined"
```

**原因**：`Nm_OsIf.h` 在编译期检查 `Nm_EnterCritical` 和 `Nm_ExitCritical` 两个宏是否已定义。未定义时触发 `#error`。

**解决**：在 `#include "Nm.h"` **之前**定义这两个宏：

```c
/* FreeRTOS */
#define Nm_EnterCritical()    taskENTER_CRITICAL()
#define Nm_ExitCritical()     taskEXIT_CRITICAL()

/* 裸机 */
#define Nm_EnterCritical()    __disable_irq()
#define Nm_ExitCritical()     __enable_irq()

#include "Nm.h"  /* ← 必须在宏之后 */
```

**必须是宏定义**（`#define`），不能是普通函数声明。

---

## Q2: 编译报 `unknown type name 'uint8'`

**错误信息**：
```
Nm_ConfigTypes.h: error: unknown type name 'uint8'
Nm.h: error: unknown type name 'uint16'
```

**原因**：NM 模块使用 `uint8`、`uint16`、`uint32`、`boolean` 等 AUTOSAR 标准类型，依赖 `Std_Types.h` 提供 typedef。

**解决**：

- **嵌入式项目**：确保 `Std_Types.h` 在编译器 include path 中
- **PC 测试**：`-DNM_HOST_TEST` 自动提供 typedef（在 `test_nm_state.c` 开头定义）：

```c
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned char  boolean;
#define TRUE   1U
#define FALSE  0U
```

如果是自己的测试文件，请确认包含了这些 typedef 或启用了 `-DNM_HOST_TEST`。

---

## Q3: ARMCC V5 编译报 `__attribute__((weak))` 语法错误

**错误信息**：
```
error: #20: identifier "__attribute__" is undefined
```

**原因**：ARMCC V5 不支持 GCC 风格的 `__attribute__((weak))` 弱符号语法。`CanNm.c` 中的 `CanNm_Transmit` 等函数使用 `NM_WEAK` 宏声明弱符号。

**解决**：`Nm_OsIf.h` 已提供跨编译器兼容的 `NM_WEAK` 宏：

```c
#if defined(__ARMCC_VERSION)        /* ARMCC V5 */
    #define NM_WEAK  __weak
#elif defined(__GNUC__)             /* GCC */
    #define NM_WEAK  __attribute__((weak))
#else
    #define NM_WEAK  /* 不支持弱符号，需提供强定义 */
#endif
```

如果你的编译器不在上述列表中，可以在 `#include "Nm_OsIf.h"` 之前自行定义：

```c
#define NM_WEAK  /* 禁用弱符号，由项目提供强定义 */
```

> ARMCC V6 / ARMCLANG 使用 `__attribute__((weak))`，GCC 兼容。

---

## Q4: 如何在同一 ECU 上使用两种 NM 类型？

**场景**：CAN0 需要 OSEK 直接 NM（逻辑环），CAN1 只需要间接 NM（监听）。

**解决**：为不同通道配置不同的 `nmMode`：

```c
static const Nm_ChannelConfigType NmChannel_CAN0 = {
    .channelHandle = 0,
    .nmMode        = NM_MODE_DIRECT,    /* CAN0: OSEK 直接 NM */
    .wireConfig    = { .wireFormat = NM_WIRE_FORMAT_OPCODE, .singleCanId = 0x500, ... },
    .timerTyp      = 1000,
    .timerMax      = 2000,
    .timerError    = 1000,
    .timerWaitBusSleep = 5000,
    /* ... 其他 Direct 相关参数 ... */
};

static const Nm_ChannelConfigType NmChannel_CAN1 = {
    .channelHandle = 1,
    .nmMode        = NM_MODE_INDIRECT,  /* CAN1: OSEK 间接 NM */
    .wireConfig    = { .wireFormat = NM_WIRE_FORMAT_OPCODE, .singleCanId = 0x600, ... },
    .timerMax      = 2000,              /* 仅需 TMax + TWbs */
    .timerWaitBusSleep = 5000,
    /* Indirect NM 不需要 Alive/Ring/LimpHome 参数 */
};

static const Nm_ChannelConfigType* channels[] = { &NmChannel_CAN0, &NmChannel_CAN1 };
const Nm_ConfigType NmConfig = {
    .numChannels           = 2,
    .channels              = channels,
    .mainFunctionPeriodMs  = 5,
};
```

`CanNm.c` 会为每个通道初始化对应的 vtable 实例，运行时通过 `ctx->vtable->xxx()` 分发。两个通道完全独立运行，互不干扰。

---

## Q5: AUTOSAR NM 什么时候可以完整使用？

**当前状态**：`CanNm_Autosar.c` 是**骨架实现**，包含：

- ✅ 完整的 7 状态状态机框架
- ✅ vtable 桩函数（18 个，编译可通过）
- ✅ CBV (Control Bit Vector) 数据结构定义
- 🟡 核心逻辑标记为 `/* TODO: AUTOSAR NM */`

**待实现功能**（搜索 `TODO` 即可定位）：

| 功能 | 说明 |
|------|------|
| CBV 完整构造与解析 | RepeatMsgReq / ActiveWakeup / PN / CoordinatorSleep bit |
| Repeat Message 计数器 | 递减至 0 的逻辑 |
| 协调器模式 | 多节点睡眠同步 |
| Partial Networking (PN) | PN 过滤器 |

**当前可用于**：集成编译验证、vtable 架构验证。完整功能需后续开发填充。

---

## Q6: 定时器数量不够怎么办 (`NM_TIMER_MAX_COUNT`)

**现象**：多通道、多定时器同时运行时，定时器池耗尽，导致新定时器分配失败。

**默认值**：`NM_TIMER_MAX_COUNT = 40`（定义在 `Nm_Internal.h` 或 `Nm_Timer.h`）。

**每个通道的定时器消耗**：

| NM 模式 | 大致定时器数 |
|---------|:----------:|
| OSEK Direct | 5 个 (hTTyp / hTMax / hTError / hTWbs / hTTx) |
| OSEK Indirect | 2 个 (hToB / hTWbs) |
| AUTOSAR NM | 3 个 (hNmTimeout / hNmRepeatMsg / hNmWaitBusSleep) |
| CFMOTO 扩展 | +3 个 |

**解决**：

1. **增加池大小**：在 `#include "Nm.h"` 之前定义：

```c
#define NM_TIMER_MAX_COUNT  80   /* 默认 40, 按需增大 */
#include "Nm.h"
```

2. **如果内存紧张**：检查是否有多余的通道配置，减少通道数

3. **确认定时器正确释放**：每个 `Nm_Timer_Start` 应有对应的 `Nm_Timer_Stop` 或自动期满释放

---

## Q7: PDU 8 字节不够用怎么办

**默认值**：`NM_PDU_MAX_LENGTH = 8`（标准 CAN 帧最大数据长度）。

**PDU 布局**（OpCode 模式）：

| 字节 | 0 | 1 | 2-7 |
|------|---|---|-----|
| 内容 | OpCode | Source Node ID | User Data (最多 6 字节) |

**解决**：

- **标准场景（8 字节）**：无需修改，6 字节 User Data 足够
- **CAN FD 场景（64 字节）**：修改 `Nm_ConfigTypes.h` 中的 `NM_PDU_MAX_LENGTH`：

```c
#define NM_PDU_MAX_LENGTH  64U   /* CAN FD 支持 */
```

- **注意**：增大 PDU 长度会影响 `g_lastTxPdu` 数组大小和 `CanNm_Transmit` 的缓冲区，以及所有 `Nm_RxIndication` 的 PDU 参数

---

## 其他常见问题

### Q: 链接报 `undefined reference to Nm_NetworkMode`

**原因**：`Nm_Cbk.h` 中声明的回调函数未实现。

**解决**：在应用层为所有 12 个回调提供实现（至少空函数体）：

```c
void Nm_NetworkMode(NetworkHandleType ch)              { (void)ch; }
void Nm_PrepareBusSleepMode(NetworkHandleType ch)      { (void)ch; }
/* ... 其他 10 个 ... */
```

### Q: `Nm_GetState` 返回的状态不对

**原因**：`Nm_MainFunction()` 调用周期与 `mainFunctionPeriodMs` 不匹配。

**解决**：确保 `Nm_MainFunction()` 以配置的周期调用：

```c
/* mainFunctionPeriodMs = 5 时 */
while (1) {
    Nm_MainFunction();
    vTaskDelay(pdMS_TO_TICKS(5));  /* = mainFunctionPeriodMs */
}
```

### Q: CAN 抓包显示 NM 报文时序不对

**原因**：`timerTyp` / `timerMax` 等参数未按节点数合理设置。

**参考值**（6 节点逻辑环）：

| 参数 | 推荐值 |
|------|:------:|
| `timerTyp` | 1000ms |
| `timerMax` | 2000ms |
| `timerError` | 1000ms |
| `timerWaitBusSleep` | 5000ms |
| `timerTx` | 100ms |

节点数越多，`timerMax` 应相应增大（确保环上每个节点都有时间发送 Ring 消息）。

---

> 移植完整步骤见 [[移植指南|移植指南]]。基础概念见 [[../01_概述/NM标准化模块简介|NM标准化模块简介]]。
