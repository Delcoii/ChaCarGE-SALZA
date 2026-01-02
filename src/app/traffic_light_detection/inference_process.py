import cv2
import numpy as np
import onnxruntime as ort
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

def preprocess(img):
    # Resize input image to model input size
    img = cv2.resize(img, (480, 480))
    # Convert BGR (OpenCV) to RGB
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    # Normalize pixel values to [0, 1]
    img = img.astype(np.float32) / 255.0
    # Change data layout from HWC to CHW
    img = np.transpose(img, (2, 0, 1))
    # Add batch dimension
    return np.expand_dims(img, axis=0)

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

    print(f"[Inference] POSIX MQ ready: {MQ_NAME}")

    # =========================
    # ONNX Runtime initialization
    # =========================
    session = ort.InferenceSession(
        "best_480.onnx",
        # "best_480_int8.onnx",
        providers=["CPUExecutionProvider"]
    )
    input_name = session.get_inputs()[0].name

    while True:
        # Copy frame from shared memory
        frame = frame_array.copy()
        start = time.perf_counter()

        # Preprocess input frame
        input_tensor = preprocess(frame)

        # Run inference
        output = session.run(None, {input_name: input_tensor})[0]
        output = np.squeeze(output)

        # YOLOv8 output shape is usually (84, 8400) -> 4 box coords + 80 classes (here 3 classes)
        # Transpose to (8400, 7) for easier handling if needed, or just work with cols
        # output format: [cx, cy, w, h, class0_conf, class1_conf, class2_conf]
        
        # Extract class confidence scores
        # output shape: (4 + num_classes, num_anchors)
        scores = output[4:, :]
        
        # Find best detection
        best_idx = np.argmax(scores.max(axis=0))
        best_conf = scores[:, best_idx].max()
        best_class = np.argmax(scores[:, best_idx])

        # =========================
        # Determine traffic_sign_state & Visualize
        # =========================
        traffic_sign_state = 0 # Default: NONE

        if best_conf > CONF_TH:
            traffic_sign_state = CLASS_MAP.get(best_class, 0)
            
            # --- Visualization Logic ---
            # 1. Get box coordinates (normalized to 480x480 model input)
            cx = output[0, best_idx]
            cy = output[1, best_idx]
            w = output[2, best_idx]
            h = output[3, best_idx]

            # 2. Scale coordinates back to original image (640x480)
            # Model was trained on 480x480. Input image was resized from 640x480 to 480x480.
            # So x-axis scale factor = 640 / 480, y-axis scale factor = 480 / 480 = 1
            scale_x = target_frame_x / 480
            scale_y = target_frame_y / 480

            # Calculate top-left corner
            x1 = int((cx - w/2) * scale_x)
            y1 = int((cy - h/2) * scale_y)
            x2 = int((cx + w/2) * scale_x)
            y2 = int((cy + h/2) * scale_y)

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
        cv2.imshow("Detection Result", frame)
        if cv2.waitKey(1) == ord('q'):
            break

        print(
            f"[Inference] traffic_sign_state={traffic_sign_state} "
            f"(conf={best_conf:.2f}, {infer_ms:.1f}ms)"
        )
