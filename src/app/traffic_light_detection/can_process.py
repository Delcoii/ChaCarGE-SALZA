import posix_ipc
import struct
import time

MQ_NAME = "/traffic_sign_state"

STATE_STR = {
    0: "NONE",
    1: "RED",
    2: "YELLOW",
    3: "GREEN"
}

def can_process():
    # =========================
    # Open POSIX Message Queue
    # =========================
    mq = posix_ipc.MessageQueue(MQ_NAME)

    print(f"[CAN] POSIX MQ opened: {MQ_NAME}")

    while True:
        try:
            # Blocking receive (blocking is OK for validation/testing)
            msg, _ = mq.receive()
            traffic_sign_state = struct.unpack("i", msg)[0]

            state_str = STATE_STR.get(traffic_sign_state, "UNKNOWN")

            print(f"[CAN] STATE={traffic_sign_state} ({state_str})", flush=True)

        except Exception as e:
            print(f"[CAN] MQ receive error: {e}")
            time.sleep(0.1)
