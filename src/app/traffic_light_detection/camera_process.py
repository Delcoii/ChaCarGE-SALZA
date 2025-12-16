import cv2
import time
import numpy as np
from multiprocessing import shared_memory

FRAME_W, FRAME_H, FRAME_C = 320, 240, 3

def camera_process(shm_name):
    cap = cv2.VideoCapture(1)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_W)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_H)

    shm = shared_memory.SharedMemory(name=shm_name)
    frame_array = np.ndarray((FRAME_H, FRAME_W, FRAME_C), dtype=np.uint8, buffer=shm.buf)

    frame_count = 0
    start = time.time()

    while True:
        ret, frame = cap.read()
        if not ret:
            continue

        # 최신 프레임을 공유 메모리에 복사
        frame_array[:] = frame[:FRAME_H, :FRAME_W, :]
        frame_count += 1

        # 1초마다 FPS 출력
        if time.time() - start >= 1.0:
            print(f"[Camera] FPS={frame_count}")
            frame_count = 0
            start = time.time()

        time.sleep(0.001)
