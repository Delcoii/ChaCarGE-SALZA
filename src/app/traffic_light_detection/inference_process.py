import cv2
import numpy as np
import onnxruntime as ort
import time
from multiprocessing import shared_memory
import posix_ipc
import struct

# =========================
# POSIX Message Queue specification (fixed)
# =========================
MQ_NAME = "/traffic_sign_state"

# traffic_sign_state definition
# 0: NONE, 1: RED, 2: YELLOW, 3: GREEN
CLASS_MAP = {
    0: 3,  # green
    1: 1,  # red
    2: 2   # yellow
}

# Confidence threshold for valid detection
CONF_TH = 0.25

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
        (240, 320, 3),
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

        # Extract class confidence scores
        scores = output[4:, :]
        best_idx = np.argmax(scores.max(axis=0))
        best_conf = scores[:, best_idx].max()
        best_class = np.argmax(scores[:, best_idx])

        # =========================
        # Determine traffic_sign_state
        # =========================
        if best_conf > CONF_TH:
            traffic_sign_state = CLASS_MAP[best_class]
        else:
            traffic_sign_state = 0  # NONE

        # =========================
        # Send result via POSIX Message Queue
        # =========================
        mq.send(struct.pack("i", traffic_sign_state))

        infer_ms = (time.perf_counter() - start) * 1000

        print(
            f"[MQ SEND] traffic_sign_state={traffic_sign_state} "
            f"(conf={best_conf:.2f}, {infer_ms:.1f}ms)"
        )
