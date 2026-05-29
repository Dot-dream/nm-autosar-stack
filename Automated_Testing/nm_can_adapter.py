"""NM CAN 适配层 — 封装 USB2CAN 操作为高层 NM 测试 API"""
import sys
import time
from ctypes import byref

sys.path.insert(0, "USB2CAN")
from usb_device import *
from usb2can import *

CAN_CMD_ID = 0x7E0

CMD_NETWORK_REQUEST      = 0x01
CMD_NETWORK_RELEASE       = 0x02
CMD_DISABLE_COMMUNICATION = 0x03
CMD_ENABLE_COMMUNICATION  = 0x04
CMD_CONTROLLER_BUSOFF     = 0x05
CMD_IG_ON                 = 0x06
CMD_IG_OFF                = 0x07


class NmCanAdapter:
    def __init__(self, cfg: dict):
        self._dev = c_uint(0)
        self._ch = cfg["can_device"]["channel"]
        self._timeout = cfg["can_device"].get("timeout_ms", 5000)
        self._cfg = cfg

        # 扫描设备
        handles = (c_uint * 20)()
        n = USB_ScanDevice(byref(handles))
        if n == 0:
            raise RuntimeError("未检测到 USB2CAN 设备")
        self._dev = handles[0]
        print(f"[CAN] 检测到 {n} 个设备, 打开设备 0")

        if not USB_OpenDevice(self._dev):
            raise RuntimeError("打开设备失败")

        # 配置波特率
        can_cfg = CAN_INIT_CONFIG()
        ret = CAN_GetCANSpeedArg(self._dev, byref(can_cfg), cfg["can_bitrate"])
        if ret != CAN_SUCCESS:
            raise RuntimeError(f"获取波特率参数失败 (ret={ret})")
        ret = CAN_Init(self._dev, self._ch, byref(can_cfg))
        if ret != CAN_SUCCESS:
            raise RuntimeError(f"CAN 初始化失败 (ret={ret})")

        # 过滤器 — 接收所有
        flt = CAN_FILTER_CONFIG()
        flt.Enable = 1
        flt.ExtFrame = 0
        flt.FilterIndex = 0
        flt.FilterMode = 0
        flt.MASK_IDE = 0
        flt.MASK_RTR = 0
        flt.MASK_Std_Ext = 0
        CAN_Filter_Init(self._dev, self._ch, byref(flt))
        print(f"[CAN] 初始化完成, 波特率 {cfg['can_bitrate']} bps")

    def send_msg(self, can_id: int, data: list) -> bool:
        msg = (CAN_MSG * 1)()
        msg[0].ID = can_id
        msg[0].ExternFlag = 0
        msg[0].RemoteFlag = 0
        msg[0].DataLen = len(data)
        for i, b in enumerate(data):
            msg[0].Data[i] = b
        n = CAN_SendMsg(self._dev, self._ch, byref(msg), 1)
        return n >= 0

    def send_cmd(self, cmd: int) -> bool:
        return self.send_msg(self._cfg["dut"]["can_id_cmd"], [cmd])

    def flush_rx(self):
        buf = (CAN_MSG * 10240)()
        CAN_GetMsg(self._dev, self._ch, byref(buf))

    def recv_msg(self, timeout_ms: int) -> dict | None:
        deadline = time.time() + timeout_ms / 1000.0
        while time.time() < deadline:
            buf = (CAN_MSG * 256)()
            n = CAN_GetMsg(self._dev, self._ch, byref(buf))
            if n > 0:
                data = [buf[0].Data[i] for i in range(buf[0].DataLen)]
                return {
                    "id": buf[0].ID,
                    "ts": buf[0].TimeStamp,
                    "data": data,
                    "dlc": buf[0].DataLen,
                }
            time.sleep(0.001)
        return None

    def wait_msg(self, can_id: int, opcode: int, timeout_ms: int) -> dict | None:
        """等待 OSEK Direct NM OpCode 匹配 (mask 0x3F, 忽略 ACTIVE bit 0x40)"""
        deadline = time.time() + timeout_ms / 1000.0
        while time.time() < deadline:
            msg = self.recv_msg(200)
            if msg and msg["id"] == can_id and len(msg["data"]) > 0:
                if (msg["data"][0] & 0x3F) == opcode:
                    return msg
        return None

    def wait_msg_cbv(self, can_id: int, bit_mask: int, bit_val: int, timeout_ms: int) -> dict | None:
        deadline = time.time() + timeout_ms / 1000.0
        while time.time() < deadline:
            msg = self.recv_msg(200)
            if msg and msg["id"] == can_id and len(msg["data"]) > 1:
                cbv = msg["data"][1]
                if (cbv & bit_mask) == bit_val:
                    return msg
        return None

    def wait_any_msg(self, can_id: int, timeout_ms: int) -> dict | None:
        deadline = time.time() + timeout_ms / 1000.0
        while time.time() < deadline:
            msg = self.recv_msg(200)
            if msg and msg["id"] == can_id:
                return msg
        return None

    def wait_silence(self, can_id: int, duration_ms: int) -> bool:
        """确认 duration_ms 内无指定 CAN ID 消息"""
        deadline = time.time() + duration_ms / 1000.0
        while time.time() < deadline:
            remain = (deadline - time.time()) * 1000
            if remain <= 0:
                break
            msg = self.recv_msg(min(500, int(remain)))
            if msg and msg["id"] == can_id:
                return False
        return True

    def close(self):
        USB_CloseDevice(self._dev)
        print("[CAN] 设备已关闭")
