import cv2
import numpy as np
from ultralytics import YOLO
import time
from multiprocessing import shared_memory
import posix_ipc
import struct

# target_frame_x = 640
# target_frame_y = 480
target_frame_x = 320
target_frame_y = 240

# =========================
# POSIX Message Queue specification (fixed)
# =========================
MQ_NAME = "/traffic_sign_state"

# traffic_sign_state definition
# 0: NONE, 1: RED, 2: YELLOW, 3: GREEN
# Model Output Class Index -> System State Code
CLASS_MAP = {
    0: 3,  # green
    1: 1,  # red
    2: 2   # yellow
}

# Visualization Labels
LABEL_MAP = {
    0: "GREEN",
    1: "RED",
    2: "YELLOW"
}

# Colors for visualization (BGR)
COLOR_MAP = {
    0: (0, 255, 0),    # Green
    1: (0, 0, 255),    # Red
    2: (0, 255, 255)   # Yellow
}

# Confidence threshold for valid detection
CONF_TH = 0.50

def inference_process(shm_name):
    # =========================
    # Shared Memory setup
    # =========================
    shm = shared_memory.SharedMemory(name=shm_name)
    frame_array = np.ndarray(
        (target_frame_y, target_frame_x, 3),
        dtype=np.uint8,
        buffer=shm.buf
    )

    # =========================
    # POSIX Message Queue setup
    # =========================
    mq = posix_ipc.MessageQueue(
        MQ_NAME,
        flags=posix_ipc.O_CREAT,
        mode=0o666,
        max_messages=10,
        max_message_size=4   # sizeof(int)
    )

    print(f"[Inference PT] POSIX MQ ready: {MQ_NAME}")

    # =========================
    # YOLO PyTorch Model initialization
    # =========================
    # Load the YOLOv8 model
    try:
        model = YOLO("best.pt")
        print("[Inference PT] YOLOv8 model loaded successfully: best.pt")
    except Exception as e:
        print(f"[Inference PT] Error loading model: {e}")
        return

    # Create a named window with WINDOW_NORMAL to allow resizing
    cv2.namedWindow("Detection Result (PT)", cv2.WINDOW_NORMAL)

    while True:
        # Copy frame from shared memory
        frame = frame_array.copy()
        start = time.perf_counter()

        # Run inference
        # YOLOv8 handles preprocessing (resizing, normalization) internally
        # verbose=False prevents printing to console every frame
        results = model(frame, verbose=False)

        # =========================
        # Determine traffic_sign_state & Visualize
        # =========================
        traffic_sign_state = 0 # Default: NONE
        best_conf = 0.0
        best_box = None
        best_class = -1

        # Check if there are any detections
        if results[0].boxes:
            # Iterate through detections to find the best one
            for box in results[0].boxes:
                conf = float(box.conf)
                cls = int(box.cls)
                
                # Filter by confidence
                if conf > CONF_TH and conf > best_conf:
                    # [Optional] Filter by box size to avoid false positives on large objects
                    # xyxy format: [x1, y1, x2, y2]
                    xyxy = box.xyxy[0].cpu().numpy()
                    w = xyxy[2] - xyxy[0]
                    h = xyxy[3] - xyxy[1]
                    
                    # Filter if box is too large (e.g., > 80% of frame width/height)
                    if w > target_frame_x * 0.8 or h > target_frame_y * 0.8:
                        continue

                    best_conf = conf
                    best_class = cls
                    best_box = xyxy

        if best_class != -1:
            traffic_sign_state = CLASS_MAP.get(best_class, 0)
            
            # --- Visualization Logic ---
            x1, y1, x2, y2 = map(int, best_box)

            # Draw Box
            color = COLOR_MAP.get(best_class, (255, 255, 255))
            cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
            
            # Draw Label
            label = f"{LABEL_MAP.get(best_class, 'Unknown')} {best_conf:.2f}"
            cv2.putText(frame, label, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)

        # =========================
        # Send result via POSIX Message Queue
        # =========================
        mq.send(struct.pack("i", traffic_sign_state))

        infer_ms = (time.perf_counter() - start) * 1000

        # Draw inference time
        cv2.putText(frame, f"Infer: {infer_ms:.1f}ms", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

        # Show Result
        cv2.imshow("Detection Result (PT)", frame)
        if cv2.waitKey(1) == ord('q'):
            break

        if traffic_sign_state != 0:
            print(
                f"[Inference PT] traffic_sign_state={traffic_sign_state} "
                f"(conf={best_conf:.2f}, {infer_ms:.1f}ms)"
            )


