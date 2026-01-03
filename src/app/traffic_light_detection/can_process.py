import posix_ipc
import struct
import time
import can

# =========================
# POSIX MQ settings
# =========================
MQ_NAME = "/traffic_sign_state"

STATE_STR = {
    0: "NONE",
    1: "RED",
    2: "YELLOW",
    3: "GREEN"
}

# =========================
# CAN settings
# =========================
CAN_CHANNEL = "can0"
CAN_ID = 0x123  # Standard 11-bit ID

def can_process():
    # =========================
    # Open POSIX Message Queue
    # =========================
    mq = posix_ipc.MessageQueue(MQ_NAME)
    print(f"[CAN][INIT] POSIX MQ opened: {MQ_NAME}", flush=True)

    # =========================
    # Open CAN bus
    # =========================
    bus = can.interface.Bus(
        channel=CAN_CHANNEL,
        bustype="socketcan"
    )
    print(f"[CAN][INIT] CAN bus opened on {CAN_CHANNEL}", flush=True)

    print("[CAN][RUN] Waiting for messages...", flush=True)

    while True:
        try:
            # =========================
            # 1. Receive from POSIX MQ
            # =========================
            msg, prio = mq.receive()
            recv_ts = time.time()

            traffic_sign_state = struct.unpack("i", msg)[0]
            state_str = STATE_STR.get(traffic_sign_state, "UNKNOWN")

            # print(
            #     f"[CAN][MQ RX] ts={recv_ts:.3f} "
            #     f"raw={msg} "
            #     f"state={traffic_sign_state} ({state_str})",
            #     flush=True
            # )

            # =========================
            # 2. Build CAN frame
            # =========================
            can_msg = can.Message(
                arbitration_id=CAN_ID,
                data=[traffic_sign_state],
                is_extended_id=False
            )

            # print(
            #     f"[CAN][BUILD] ID=0x{CAN_ID:X} "
            #     f"DLC=1 DATA=[{traffic_sign_state}]",
            #     flush=True
            # )

            # =========================
            # 3. Send CAN frame
            # =========================
            bus.send(can_msg)
            send_ts = time.time()

            # no duration!
            # print(
            #     f"[CAN] op_time = {send_ts - recv_ts:.3f} "
            #     f"STATE={traffic_sign_state} ({state_str})",
            #     flush=True
            # )

        except can.CanError as e:
            print(f"[CAN][TX FAIL] CAN send error: {e}", flush=True)
            time.sleep(0.1)

        except Exception as e:
            print(f"[CAN][ERROR] MQ receive error: {e}", flush=True)
            time.sleep(0.1)
