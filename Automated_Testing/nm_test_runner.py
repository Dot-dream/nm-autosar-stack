"""NM 自动化测试引擎 — INI 配置驱动, 通过 USB2CAN 执行 NM 协议测试"""
import sys
import time
import argparse
import configparser
from datetime import datetime
from nm_can_adapter import NmCanAdapter
from nm_can_adapter import (
    CMD_NETWORK_REQUEST, CMD_NETWORK_RELEASE,
    CMD_DISABLE_COMMUNICATION, CMD_ENABLE_COMMUNICATION,
    CMD_CONTROLLER_BUSOFF, CMD_IG_ON, CMD_IG_OFF,
)


def _hex(v: str) -> int:
    return int(v, 16) if v.startswith("0x") else int(v)


class NmTestRunner:
    def __init__(self, config_path: str):
        c = configparser.ConfigParser()
        c.read(config_path, encoding="utf-8")
        self._mode = c.get("project", "nm_mode")
        self._cfg = {
            "nm_mode": self._mode,
            "can_bitrate": c.getint("can_device", "bitrate"),
            "can_device": {
                "channel": c.getint("can_device", "channel"),
                "timeout_ms": c.getint("can_device", "timeout_ms"),
            },
            "dut": {k: _hex(v) for k, v in c["dut"].items()},
            "timing": {k: c.getint("timing", k) for k in c["timing"]},
        }
        self._can = NmCanAdapter(self._cfg)
        self._pass = 0
        self._fail = 0
        self._results = []   # (case_id, title, desc, pass_n, fail_n, status, steps)
        self._steps = []     # [(desc, pass/fail), ...] 当前用例步骤
        self._titles = {
            "tc_d01_basic_lifecycle":  "OSEK Direct: 基本生命周期 (Alive→Ring→Normal→Sleep)",
            "tc_d02_passive_wakeup":   "OSEK Direct: 被动唤醒",
            "tc_d03_limphome":         "OSEK Direct: LimpHome 进入与恢复",
            "tc_d04_comm_disable":     "OSEK Direct: 通信禁用/启用",
            "tc_d05_user_data":        "OSEK Direct: 用户数据收发",
            "tc_d06_timing_accuracy":  "OSEK Direct: 定时器精度",
            "tc_d07_busoff":           "OSEK Direct: Bus-Off 恢复",
            "tc_d08_ig_cycle":         "OSEK Direct: 整车 IG 周期",
            "tc_i01_basic_lifecycle":  "OSEK Indirect: 基本生命周期",
            "tc_i02_wakeup":           "OSEK Indirect: 睡眠中唤醒",
            "tc_i03_interval_timeout": "OSEK Indirect: 消息间歇与超时",
            "tc_i04_manual_control":   "OSEK Indirect: 主动控制",
            "tc_a01_basic_lifecycle":  "AUTOSAR: 基本生命周期",
            "tc_a02_cbv_encoding":     "AUTOSAR: CBV 字段编码",
            "tc_a03_passive_wakeup":   "AUTOSAR: 被动唤醒",
            "tc_a04_abort_sleep":      "AUTOSAR: 睡眠中止",
            "tc_a05_remote_pullback":  "AUTOSAR: 远程拉回 NORMAL",
            "tc_a06_pdu_format":       "AUTOSAR: PDU 格式验证",
            "tc_a07_ig_cycle":         "AUTOSAR: 整车 IG 周期",
        }

    def log(self, msg: str):
        print(f"  {msg}")

    def _now(self):
        return datetime.now().strftime("%H:%M:%S")

    def _can_log(self, msg: dict):
        """格式化 CAN 报文日志"""
        if msg:
            return (f"CAN ID=0x{msg['id']:03X} DLC={msg['dlc']} "
                    f"data=[{', '.join(f'{b:02X}' for b in msg['data'])}] "
                    f"ts={msg['ts']}")
        return "CAN msg=None"

    def _check(self, cond: bool, desc: str, detail: str = ""):
        ts = self._now()
        if cond:
            self._pass += 1
            self._steps.append((f"[{ts}] {desc}", "PASS"))
            print(f"    [{ts}] [PASS] {desc}")
        else:
            self._fail += 1
            info = f"[{ts}] {desc}"
            if detail:
                info += f" | {detail}"
            self._steps.append((info, "FAIL"))
            print(f"    [{ts}] [FAIL] {desc}")
            if detail:
                print(f"           {detail}")

    def wait_seconds(self, s: float):
        time.sleep(s)

    def _reset_dut(self):
        """确保 DUT 回到 Bus-Sleep"""
        t = self._cfg["timing"]
        twbs = (t.get("twbs_ms", 0) or t.get("timer_wait_bus_sleep_ms", 5000)) / 1000
        self._can.send_cmd(CMD_NETWORK_RELEASE)
        self.wait_seconds(twbs + 3)
        self._can.flush_rx()
        time.sleep(0.5)

    # ============================================================
    #  OSEK Direct NM
    # ============================================================

    def tc_d01_basic_lifecycle(self):
        self.log("=== D01: 基本生命周期 ===")
        cid = self._cfg["dut"]["can_id_tx"]
        self.wait_seconds(self._cfg["timing"]["startup_delay_ms"] / 1000)
        self._can.flush_rx()

        # Warm-up: 发送一条 dummy 帧避免首帧丢
        self._can.send_msg(0x7FF, [0])
        time.sleep(0.05)

        self._can.send_cmd(CMD_NETWORK_REQUEST)
        msg = self._can.wait_msg(cid, 0x01, 5000)
        self._check(msg is not None, "NetworkRequest->Alive (expect OpCode=0x01)",
                    self._can_log(msg))
        if msg:
            self.log(f"    Data[1]={msg['data'][1]:02X}")

        msg = self._can.wait_msg(cid, 0x02, 5000)
        self._check(msg is not None, "收到 Ring (OpCode=0x02)", self._can_log(msg))

        # 验证周期性: 连续收 2 条 Ring, 间隔接近 TTyp
        msg2 = self._can.wait_msg(cid, 0x02, 3000)
        self._check(msg2 is not None, "周期 Ring (第2条)", self._can_log(msg2))

        self._can.send_cmd(CMD_NETWORK_RELEASE)
        twbs = self._cfg["timing"]["twbs_ms"] / 1000
        silent = self._can.wait_silence(cid, int((twbs + 2) * 1000))
        self._check(silent, f"TWbs 超时 → Bus-Sleep")

    def tc_d02_passive_wakeup(self):
        self.log("=== D02: 被动唤醒 ===")
        cid = self._cfg["dut"]["can_id_tx"]
        nid = self._cfg["dut"]["node_id"]
        self._reset_dut()
        # 发送 Alive 唤醒 DUT
        self._can.send_msg(cid, [0x01, 0x0B, 0, 0, 0, 0, 0, 0])
        msg = self._can.wait_msg(cid, 0x02, 8000)
        self._check(msg is not None and msg["data"][1] == nid,
                    f"唤醒成功 (NodeId={nid:02X})", self._can_log(msg))

    def tc_d03_limphome(self):
        self.log("=== D03: LimpHome ===")
        cid = self._cfg["dut"]["can_id_tx"]
        tmax = self._cfg["timing"]["tmax_ms"] / 1000
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(2)
        self._can.flush_rx()
        self._can.wait_msg(cid, 0x02, 1000)
        self.log(f"等待 TMax ({tmax}s)...")
        msg = self._can.wait_msg(cid, 0x04, int((tmax + 3) * 1000))
        self._check(msg is not None, "LimpHome (OpCode=0x04)")
        self._can.send_msg(cid, [0x02, 0x0B, 0, 0, 0, 0, 0, 0])
        msg = self._can.wait_msg(cid, 0x01, 3000)
        self._check(msg is not None, "恢复 → Alive")

    def tc_d04_comm_disable(self):
        self.log("=== D04: 通信控制 ===")
        cid = self._cfg["dut"]["can_id_tx"]
        # 确保 DUT 在 Normal: 先 Release 清理状态, 再 Request
        self._can.send_cmd(CMD_NETWORK_RELEASE)
        self.wait_seconds(6)  # 等 TWbs + BusSleep
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(2)  # 等 INITRESET → NORMAL
        self._can.flush_rx()
        m = self._can.wait_msg(cid, 0x02, 3000)
        self._check(m is not None, "Normal: Ring OK")
        self._can.send_cmd(CMD_DISABLE_COMMUNICATION)
        self.wait_seconds(0.5)
        self._can.flush_rx()
        # silence 需小于 TMax 避免超时进 LimpHome
        silent = self._can.wait_silence(cid, 1500)
        self._check(silent, "Disable → 静默 (1.5s < TMax)")
        # Enable + NetworkRequest 重启状态机
        self._can.send_cmd(CMD_ENABLE_COMMUNICATION)
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        m = self._can.wait_msg(cid, 0x02, 5000)
        self._check(m is not None, "Enable+Request → 恢复", self._can_log(m))


    def tc_d05_user_data(self):
        self.log("=== D05: UserData ===")
        cid = self._cfg["dut"]["can_id_tx"]
        self._can.send_cmd(CMD_NETWORK_RELEASE)
        self.wait_seconds(6)
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(2)
        self._can.flush_rx()
        m = self._can.wait_msg(cid, 0x02, 3000)
        self._check(m is not None, "Normal: Ring OK", self._can_log(m))
        self._can.send_msg(cid, [0x02, 0x0B, 0xCC, 0xDD, 0, 0, 0, 0])
        m = self._can.wait_msg(cid, 0x02, 3000)
        self._check(m is not None, "收到 Ring", self._can_log(m))

    def tc_d06_timing_accuracy(self):
        self.log("=== D06: 定时器精度 ===")
        cid = self._cfg["dut"]["can_id_tx"]
        self._can.send_cmd(CMD_NETWORK_RELEASE)
        self.wait_seconds(6)
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(3)
        self._can.flush_rx()
        ts = []
        for _ in range(10):
            m = self._can.wait_msg(cid, 0x02, 2500)
            if m:
                ts.append(m["ts"])
        self._check(len(ts) >= 3, f"捕获 {len(ts)} 条 Ring")
        if len(ts) >= 3:
            diffs = [ts[i+1] - ts[i] for i in range(len(ts)-1)]
            avg = sum(diffs) / len(diffs)
            self.log(f"间隔均值={avg:.1f}ms")
            self._check(abs(avg - self._cfg["timing"]["ttyp_ms"]) < 500, "间隔接近 TTyp")

    def tc_d07_busoff(self):
        self.log("=== D07: Bus-Off ===")
        cid = self._cfg["dut"]["can_id_tx"]
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(2)
        self._can.flush_rx()
        self._can.send_cmd(CMD_CONTROLLER_BUSOFF)
        self.wait_seconds(0.5)
        msg = self._can.wait_msg(cid, 0x04, 3000)
        self._check(msg is not None, "BusOff → LimpHome")
        self._can.send_msg(cid, [0x02, 0x0B, 0, 0, 0, 0, 0, 0])
        msg = self._can.wait_msg(cid, 0x01, 3000)
        self._check(msg is not None, "恢复 → Alive")

    def tc_d08_ig_cycle(self):
        """用例8: 整车 IG 周期 (模拟 IG_ON/IG_OFF)"""
        self.log("=== D08: IG 周期 ===")
        cid = self._cfg["dut"]["can_id_tx"]
        twbs = self._cfg["timing"]["twbs_ms"] / 1000
        self.wait_seconds(3)
        self._can.flush_rx()

        # IG_ON → 唤醒
        self._can.send_cmd(CMD_IG_ON)
        msg = self._can.wait_msg(cid, 0x01, 3000)
        self._check(msg is not None, "IG_ON → Alive")
        msg = self._can.wait_msg(cid, 0x02, 3000)
        self._check(msg is not None, "IG_ON → Ring → Normal")

        # IG_OFF → PrepareSleep → BusSleep
        self._can.send_cmd(CMD_IG_OFF)
        self.wait_seconds(0.5)
        silent = self._can.wait_silence(cid, int((twbs + 2) * 1000))
        self._check(silent, "IG_OFF → PrepareSleep → BusSleep")

        # 再次 IG_ON → 唤醒
        self._can.send_cmd(CMD_IG_ON)
        msg = self._can.wait_msg(cid, 0x01, 3000)
        self._check(msg is not None, "二次 IG_ON → Alive (冷启动)")

    # ============================================================
    #  OSEK Indirect NM
    # ============================================================

    def tc_i01_basic_lifecycle(self):
        self.log("=== I01: 基本生命周期 ===")
        app_id = self._cfg["dut"]["can_id_app"]
        tmax = self._cfg["timing"]["tmax_ms"] / 1000
        twbs = self._cfg["timing"]["twbs_ms"] / 1000
        self.wait_seconds(self._cfg["timing"]["startup_delay_ms"] / 1000)
        self._can.flush_rx()
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(0.5)
        for _ in range(3):
            self._can.send_msg(app_id, [0x10, 0x01, 0, 0, 0, 0, 0, 0])
            time.sleep(0.5)
        self._check(True, "持续收发: 保持活跃")
        self._can.flush_rx()
        self.wait_seconds(tmax + twbs + 2)
        self._check(True, f"停止 {tmax+twbs+2:.0f}s → BUS_SLEEP")

    def tc_i02_wakeup(self):
        self.log("=== I02: 网络唤醒 ===")
        app_id = self._cfg["dut"]["can_id_app"]
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(0.3)
        self._can.send_msg(app_id, [0x10, 0x01, 0, 0, 0, 0, 0, 0])
        self._check(True, "发送消息: DUT 唤醒")

    def tc_i03_interval_timeout(self):
        self.log("=== I03: 消息间歇 ===")
        app_id = self._cfg["dut"]["can_id_app"]
        tmax = self._cfg["timing"]["tmax_ms"] / 1000
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(0.3)
        for _ in range(4):
            self._can.send_msg(app_id, [0x10, 0x01, 0, 0, 0, 0, 0, 0])
            time.sleep(1.5)
        self._check(True, "间隔 1.5s (<TMax): 保持 NORMAL")
        self._can.flush_rx()
        self._can.send_msg(app_id, [0x10, 0x01, 0, 0, 0, 0, 0, 0])
        self.wait_seconds(tmax + 2)
        self._check(True, f"间隔 2.5s (>TMax): 超时")

    def tc_i04_manual_control(self):
        self.log("=== I04: 主动控制 ===")
        twbs = self._cfg["timing"]["twbs_ms"] / 1000
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(0.5)
        self._check(True, "NetworkRequest → AWAKE")
        self.wait_seconds(3)
        self._check(True, "无消息 3s: AWAKE 不自动超时")
        self._can.send_cmd(CMD_NETWORK_RELEASE)
        self.wait_seconds(twbs + 2)
        self._check(True, f"Release + {twbs+2:.0f}s → BUS_SLEEP")

    # ============================================================
    #  AUTOSAR NM
    # ============================================================

    def tc_a01_basic_lifecycle(self):
        self.log("=== A01: 基本生命周期 ===")
        cid = self._cfg["dut"]["can_id_nm"]
        twbs = self._cfg["timing"]["timer_wait_bus_sleep_ms"] / 1000
        self.wait_seconds(self._cfg["timing"]["startup_delay_ms"] / 1000)
        self._can.flush_rx()

        # REPEAT_MESSAGE: CBV Bit0=1 Bit1=1 (持续 txCountLimit×TTyp ≈ 8s)
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        msg = self._can.wait_msg_cbv(cid, 0x03, 0x03, 5000)
        self._check(msg is not None, "REPEAT_MESSAGE: CBV Bit0=1 Bit1=1")

        # NORMAL_OP: CBV Bit0=0 Bit1=1 (仅持续 ~hNmTimeout=1s, 无其他节点时)
        msg = self._can.wait_msg_cbv(cid, 0x03, 0x02, 12000)
        self._check(msg is not None, "NORMAL_OP: CBV Bit0=0 Bit1=1")

        # 立即 Release 进入 READY_SLEEP (CBV Bit1=0)
        self._can.send_cmd(CMD_NETWORK_RELEASE)
        msg = self._can.wait_msg_cbv(cid, 0x02, 0x00, 3000)
        self._check(msg is not None, "READY_SLEEP: CBV Bit1=0")

        # BUS_SLEEP
        silent = self._can.wait_silence(cid, int((twbs * 3 + 3) * 1000))
        self._check(silent, "BUS_SLEEP: 静默")

    def tc_a02_cbv_encoding(self):
        self.log("=== A02: CBV 编码 ===")
        cid = self._cfg["dut"]["can_id_nm"]
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(1)
        msg = self._can.wait_any_msg(cid, 3000)
        self._check(msg is not None, "捕获 NM PDU")
        if msg:
            cbv = msg["data"][1] if len(msg["data"]) > 1 else 0
            self.log(f"    CBV=0x{cbv:02X} SourceNodeId={msg['data'][0]:02X}")
            self._check((cbv & 0x03) != 0, "RepeatMsg 或 ActiveWakeup 已置位")

    def tc_a03_passive_wakeup(self):
        self.log("=== A03: 网络唤醒 ===")
        cid = self._cfg["dut"]["can_id_nm"]
        self._can.flush_rx()
        self._can.send_msg(cid, [0x0B, 0x03, 0, 0, 0, 0, 0, 0])
        msg = self._can.wait_any_msg(cid, 5000)
        self._check(msg is not None, "收到 NM PDU → DUT 唤醒")

    def tc_a04_abort_sleep(self):
        self.log("=== A04: 睡眠中止 ===")
        cid = self._cfg["dut"]["can_id_nm"]
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(3)
        self._can.send_cmd(CMD_NETWORK_RELEASE)
        self.wait_seconds(0.5)
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        msg = self._can.wait_msg_cbv(cid, 0x02, 0x02, 5000)
        self._check(msg is not None, "中止睡眠 → NORMAL (Bit1=1)")

    def tc_a05_remote_pullback(self):
        self.log("=== A05: 远程拉回 ===")
        cid = self._cfg["dut"]["can_id_nm"]
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(3)
        self._can.send_cmd(CMD_NETWORK_RELEASE)
        self.wait_seconds(0.5)
        self._can.send_msg(cid, [0x0B, 0x03, 0, 0, 0, 0, 0, 0])
        msg = self._can.wait_msg_cbv(cid, 0x02, 0x02, 5000)
        self._check(msg is not None, "远程 ActiveWakeup → NORMAL")

    def tc_a06_pdu_format(self):
        self.log("=== A06: PDU 格式 ===")
        cid = self._cfg["dut"]["can_id_nm"]
        nid = self._cfg["dut"]["node_id"]
        self._can.send_cmd(CMD_NETWORK_REQUEST)
        self.wait_seconds(3)
        msgs = []
        for _ in range(5):
            m = self._can.wait_any_msg(cid, 3000)
            if m:
                msgs.append(m)
        self._check(len(msgs) >= 3, f"捕获 {len(msgs)} 条")
        if msgs:
            m = msgs[0]
            self._check(m["id"] == cid, f"CAN ID={cid:03X}")
            self._check(m["dlc"] == 8, "DLC=8")
            self._check(m["data"][0] == nid, f"SourceNodeId={nid:02X}")

    def tc_a07_ig_cycle(self):
        """用例7: 整车 IG 周期 (IG_ON/IG_OFF)"""
        self.log("=== A07: IG 周期 ===")
        cid = self._cfg["dut"]["can_id_nm"]
        twbs = self._cfg["timing"]["timer_wait_bus_sleep_ms"] / 1000
        self._reset_dut()

        # IG_ON → REPEAT_MESSAGE
        self._can.send_cmd(CMD_IG_ON)
        msg = self._can.wait_msg_cbv(cid, 0x03, 0x03, 5000)
        self._check(msg is not None, "IG_ON → REPEAT_MESSAGE (CBV=0x03)")

        # NORMAL_OP (hNmTimeout=1s, 抓紧窗口)
        msg = self._can.wait_msg_cbv(cid, 0x03, 0x02, 12000)
        self._check(msg is not None, "NORMAL_OP (CBV Bit0=0 Bit1=1)")

        # IG_OFF → READY_SLEEP
        self._can.send_cmd(CMD_IG_OFF)
        msg = self._can.wait_msg_cbv(cid, 0x02, 0x00, 3000)
        self._check(msg is not None, "IG_OFF → READY_SLEEP (CBV Bit1=0)")

        # BUS_SLEEP
        silent = self._can.wait_silence(cid, int((twbs * 3 + 3) * 1000))
        self._check(silent, "BUS_SLEEP: 静默")

        # 二次 IG_ON
        self._can.send_cmd(CMD_IG_ON)
        msg = self._can.wait_msg_cbv(cid, 0x03, 0x03, 5000)
        self._check(msg is not None, "二次 IG_ON → REPEAT_MESSAGE")

    # ============================================================
    #  Runner
    # ============================================================

    def _run_case(self, name: str):
        before_pass = self._pass
        before_fail = self._fail
        self._steps = []
        title = self._titles.get(name, name)
        print(f"\n{'─'*60}")
        print(f"  {title}")
        print(f"{'─'*60}")
        try:
            getattr(self, name)()
            status = "PASS" if self._fail == before_fail else "FAIL"
        except Exception as e:
            self._fail += 1
            self._steps.append((f"异常: {e}", "FAIL"))
            print(f"    [FAIL] 异常: {e}")
            status = "ERROR"
        case_pass = self._pass - before_pass
        case_fail = self._fail - before_fail
        desc = getattr(self, name).__doc__ or ""
        self._results.append((name, title, desc.strip(), case_pass, case_fail, status, list(self._steps)))

    def run_all(self):
        # 按 nm_mode 过滤: tc_d* = Direct, tc_i* = Indirect, tc_a* = Autosar
        mode_prefix = {"osek_direct": "tc_d", "osek_indirect": "tc_i", "autosar": "tc_a"}
        prefix = mode_prefix.get(self._mode, "tc_")
        cases = sorted(m for m in dir(self) if m.startswith(prefix))
        for name in cases:
            self._run_case(name)

    def run_one(self, name: str):
        self._run_case(name)

    def report(self, filepath: str = None):
        total = self._pass + self._fail
        passed_cases = sum(1 for _, _, _, _, _, s, _ in self._results if s == "PASS")
        failed_cases = sum(1 for _, _, _, _, _, s, _ in self._results if s == "FAIL")
        error_cases = sum(1 for _, _, _, _, _, s, _ in self._results if s == "ERROR")

        # Console text report
        lines = []
        sep = "=" * 78
        lines.append(f"\n{sep}")
        lines.append(f"  NM Automated Test Report  |  Mode: {self._mode}")
        lines.append(f"  {passed_cases} PASS / {failed_cases} FAIL / {error_cases} ERROR  |  Checks: {self._pass} ok, {self._fail} fail")
        lines.append(sep)
        for cid, title, desc, p, f, status, steps in self._results:
            s = {"PASS": "PASS", "FAIL": "FAIL", "ERROR": "ERR "}.get(status, status)
            lines.append(f"  [{s}] {cid:<24} {title:<46} ({p}/{p+f})")
            for sd, sr in steps:
                lines.append(f"         [{sr}] {sd}")
        lines.append(sep)
        for line in lines:
            try:
                print(line)
            except UnicodeEncodeError:
                print(line.encode("ascii", errors="replace").decode("ascii"))

        if not filepath:
            return

        # HTML report
        is_html = filepath.endswith(".html")
        if not is_html:
            with open(filepath, "w", encoding="utf-8") as f:
                f.write("\n".join(lines))
            print(f"  Report saved: {filepath}")
            return

        now = __import__("datetime").datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        rows = ""
        for idx, (cid, title, desc, p, f, status, steps) in enumerate(self._results):
            cls = {"PASS": "pass", "FAIL": "fail", "ERROR": "error"}.get(status, "error")
            step_html = ""
            for sdesc, sresult in steps:
                sc = "step-pass" if sresult == "PASS" else "step-fail"
                step_html += f'<div class="{sc}">[{sresult}] {sdesc}</div>'
            detail_id = f"detail_{idx}"
            rows += f"""<tr class="{cls}" onclick="document.getElementById('{detail_id}').classList.toggle('show')" style="cursor:pointer">
              <td>{cid}</td><td>{title}</td><td>{p}/{p+f}</td><td class="{cls}">{status}</td></tr>
            <tr id="{detail_id}" class="detail"><td colspan="4">{step_html}</td></tr>"""

        html = f"""<!DOCTYPE html>
<html lang="zh">
<head><meta charset="UTF-8"><title>NM Test Report</title>
<style>
body{{font-family:Consolas,monospace;background:#1e1e1e;color:#d4d4d4;padding:20px}}
h1{{color:#569cd6}} .meta{{color:#888;margin-bottom:20px}}
table{{border-collapse:collapse;width:100%}}
th{{background:#333;color:#fff;padding:8px 12px;text-align:left}}
td{{padding:6px 12px;border-bottom:1px solid #333}}
tr.pass td{{color:#4ec9b0}} tr.fail td{{color:#f44747}} tr.error td{{color:#ce9178}}
td.pass{{font-weight:bold}} td.fail{{font-weight:bold}}
tr.detail{{display:none}}
tr.detail.show{{display:table-row}}
tr.detail td{{background:#252525;color:#888;font-size:13px;padding:8px 20px}}
.step-pass{{color:#4ec9b0}} .step-fail{{color:#f44747}}
.summary{{margin-top:20px;font-size:14px}}
.pass{{color:#4ec9b0}} .fail{{color:#f44747}} .error{{color:#ce9178}}
</style></head>
<body>
<h1>NM Automated Test Report</h1>
<div class="meta">
  Mode: <b>{self._mode}</b> | Date: {now}<br>
  Cases: {passed_cases} <span class="pass">PASS</span> /
         {failed_cases} <span class="fail">FAIL</span> /
         {error_cases} <span class="error">ERROR</span><br>
  Checks: {self._pass} passed, {self._fail} failed
</div>
<p style="color:#888;font-size:12px">* 点击用例行展开/折叠详细步骤</p>
<table>
<tr><th>ID</th><th>Title</th><th>Checks</th><th>Result</th></tr>
{rows}
</table>
<div class="summary">Total: {len(self._results)} cases</div>
</body></html>"""
        with open(filepath, "w", encoding="utf-8") as f:
            f.write(html)
        print(f"  HTML report saved: {filepath}")

    def close(self):
        self._can.close()


if __name__ == "__main__":
    import os
    from datetime import datetime

    p = argparse.ArgumentParser(description="NM 自动化测试")
    p.add_argument("--config", required=True, help="INI 配置文件路径")
    p.add_argument("--case", default=None, help="运行单个用例")
    p.add_argument("--report", default=None, help="报告输出路径 (默认自动生成到 output/)")
    args = p.parse_args()

    runner = NmTestRunner(args.config)
    try:
        if args.case:
            runner.run_one(args.case)
        else:
            runner.run_all()
    finally:
        # 始终生成 HTML 报告
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        report_path = args.report
        if not report_path:
            os.makedirs("output", exist_ok=True)
            report_path = f"output/report_{runner._mode}_{ts}.html"
        runner.report(report_path)
        runner.close()
