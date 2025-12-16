import cv2
import numpy as np
import onnxruntime as ort
import time
from multiprocessing import shared_memory

CLASS_MAP = {0: 3, 1: 1, 2: 2}  # green=3, red=1, yellow=2
CONF_TH = 0.25

def preprocess(img):
    img = cv2.resize(img, (480, 480))
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = img.astype(np.float32) / 255.0
    img = np.transpose(img, (2, 0, 1))
    return np.expand_dims(img, axis=0)

def inference_process(shm_name, msg_queue):
    shm = shared_memory.SharedMemory(name=shm_name)
    frame_array = np.ndarray((240, 320, 3), dtype=np.uint8, buffer=shm.buf)

    session = ort.InferenceSession("best_480.onnx", providers=["CPUExecutionProvider"])
    input_name = session.get_inputs()[0].name

    total_time = 0
    count = 0

    while True:
        frame = frame_array.copy()
        infer_start = time.perf_counter()

        input_tensor = preprocess(frame)
        output = session.run(None, {input_name: input_tensor})[0]
        output = np.squeeze(output)

        scores = output[4:, :]
        best_idx = np.argmax(scores.max(axis=0))
        best_conf = scores[:, best_idx].max()
        best_class = np.argmax(scores[:, best_idx])

        infer_time_ms = (time.perf_counter() - infer_start) * 1000

        if best_conf > CONF_TH:
            signal = CLASS_MAP[best_class]
        else:
            signal = 0

        msg_queue.put({
            "signal": signal,
            "conf": float(best_conf),
            "infer_ms": infer_time_ms
        })

        # 성능 로그 (50번마다 평균 추론 시간 출력)
        total_time += infer_time_ms
        count += 1
        if count % 50 == 0:
            print(f"[Inference] Avg infer={total_time/count:.1f} ms over {count} frames")