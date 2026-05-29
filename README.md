# nm-autosar-stack

标准化汽车网络管理 (NM) 模块 — 支持 **OSEK Direct / OSEK Indirect / AUTOSAR NM** 三种协议，vtable 多态架构，附带 PC 端 Python 自动化 CAN 测试框架。

[![Test](https://img.shields.io/badge/OSEK_Direct-7/8_PASS-brightgreen)]()
[![Test](https://img.shields.io/badge/AUTOSAR_NM-5/7_PASS-green)]()
[![Lines](https://img.shields.io/badge/lines-108K-blue)]()

---

## 目录结构

```
nm-autosar-stack/
├── NM/                          # 标准化 NM 模块 (独立复用)
│   ├── Nm.h / Nm.c               ── Nm Core (vtable 分发, 状态协调)
│   ├── Nm_ConfigTypes.h          ── 配置结构体 (3 NM 模式 + 双线缆协议)
│   ├── Nm_Cbk.h                  ── 回调声明 (12 个)
│   ├── Nm_Internal.h             ── 内部类型
│   ├── CanNm/
│   │   ├── CanNm.h / CanNm.c     ── 适配层 (3×18 vtable)
│   │   ├── CanNm_Osek_Direct.c   ── OSEK 直接 NM (逻辑环)
│   │   ├── CanNm_Osek_Indirect.c ── OSEK 间接 NM (消息监控)
│   │   └── CanNm_Autosar.c       ── AUTOSAR NM (CBV 广播)
│   ├── Nm_Timer/                 ── 时间中间件
│   ├── OsIf/                     ── OS 抽象层
│   ├── test/                     ── PC 单元测试 (gcc)
│   └── 移植指南.md
│
├── Automated_Testing/            # CAN 自动化测试框架
│   ├── nm_test_runner.py         ── 测试引擎 (19 用例, HTML 报告)
│   ├── nm_can_adapter.py         ── USB2CAN 适配
│   ├── config/                   ── INI 配置 (三协议)
│   ├── doc/                      ── 测试用例 + 通讯协议
│   └── USB2CAN/                  ── DLL 封装 (usb_device/usb2can)
│
├── Demo_Project/                 # YTM32B1MC0 裸机 Demo (Keil MDK)
│   └── app/Modules/module_nm.c   ── NM 调度器任务 + CAN 命令解析
│
├── Phase1_Analysis_Report.md     # 深度分析报告 (6AQ0 vs 6ER1)
├── NM_Comparison_README.md       # 项目管理 + 架构设计文档
└── .gitignore
```

## 支持的 NM 协议

| 模式 | 协议 | 实现文件 | 状态 |
|------|------|----------|:---:|
| `NM_MODE_DIRECT` | OSEK Direct NM (逻辑环) | `CanNm_Osek_Direct.c` | ✅ 完整 |
| `NM_MODE_INDIRECT` | OSEK Indirect NM (消息监控) | `CanNm_Osek_Indirect.c` | ✅ 完整 |
| `NM_MODE_AUTOSAR` | AUTOSAR Nm 4.x (CBV 广播) | `CanNm_Autosar.c` | ✅ 完整 |

按通道配置 `nmMode` 即可切换，三种协议通过 vtable 多态共存。

## 硬件测试结果

| 协议 | 用例通过 | 检查通过率 |
|------|:---:|:---:|
| OSEK Direct NM | 7/8 | 18/19 (95%) |
| AUTOSAR NM | 5/7 | 14/18 (78%) |

测试环境：USB2CAN (UTA0503) + YTM32B1MC0 Demo 板 + 裸机调度器

## 快速开始

### PC 端测试 (无需硬件)

```bash
cd NM
gcc -std=c11 -DNM_HOST_TEST -I. -IOsIf \
  test/test_nm_state.c Nm.c \
  CanNm/CanNm.c CanNm/CanNm_Osek_Direct.c \
  CanNm/CanNm_Osek_Indirect.c CanNm/CanNm_Autosar.c \
  Nm_Timer/Nm_Timer.c -o test_nm && ./test_nm
```

### CAN 总线测试 (需硬件)

```bash
cd Automated_Testing
python -m venv venv && venv\Scripts\activate

# OSEK 直接 NM
python nm_test_runner.py --config config/nm_config_osek_direct.ini

# AUTOSAR NM
python nm_test_runner.py --config config/nm_config_autosar.ini
```

报告自动生成到 `output/report_*.html`。

### 集成到目标项目

参见 `NM/移植指南.md` — 仅需实现 3 个平台适配函数即可。

## 设计特点

- **零外部依赖**: 纯 C (C99) + Python 标准库
- **vtable 多态**: 新增 NM 类型只需 1 个 .c 文件 + vtable 条目
- **配置外置**: `const Nm_ConfigType*` 结构体一次性传入
- **可裁剪**: 14 个编译开关控制功能模块
- **PC 可测试**: `-DNM_HOST_TEST` 脱离硬件运行
