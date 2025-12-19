/* this code receive yolo_infer_state_id code so if you want to test this code you need exec yolo main.py */

import can
import time

# =========================
# CAN configuration
# =========================
CAN_CHANNEL = "can0"
CAN_ID = 0x123  # Same CAN ID as the transmitter

STATE_STR = {
    0: "NONE",
    1: "RED",
    2: "YELLOW",
    3: "GREEN"
}

def can_receiver():
    # Open CAN bus interface
    bus = can.interface.Bus(
        channel=CAN_CHANNEL,
        bustype="socketcan"
    )
    print(f"[CAN RX][INIT] CAN bus opened on {CAN_CHANNEL}", flush=True)

    print("[CAN RX][RUN] Waiting for CAN frames...", flush=True)

    last_ts = None

    # Infinite loop to receive CAN frames
    for msg in bus:
        recv_ts = time.time()

        # Parse data (transmitter sends only 1 byte)
        # Traffic light state flag
        if len(msg.data) >= 1:
            traffic_sign_state = msg.data[0]
            state_str = STATE_STR.get(traffic_sign_state, "UNKNOWN")

            # Calculate time difference (dt)
            dt = None
            if last_ts is not None:
                dt = recv_ts - last_ts
            last_ts = recv_ts

            # Print received CAN frame information
            print(
                f"[CAN RX] ts={recv_ts:.3f} "
                f"ID=0x{msg.arbitration_id:X} "
                f"STATE={traffic_sign_state} ({state_str}) "
                f"{'(dt=' + f'{dt:.3f}' + 's)' if dt is not None else ''}",
                flush=True
            )

if __name__ == "__main__":
    can_receiver()
