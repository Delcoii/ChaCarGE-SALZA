import cv2
import time
import numpy as np
from multiprocessing import shared_memory

FRAME_W, FRAME_H, FRAME_C = 320, 240, 3

def camera_process(shm_name):
    # Open camera device
    cap = cv2.VideoCapture(0)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_W)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_H)

    # Attach to shared memory
    shm = shared_memory.SharedMemory(name=shm_name)
    frame_array = np.ndarray(
        (FRAME_H, FRAME_W, FRAME_C),
        dtype=np.uint8,
        buffer=shm.buf
    )

    frame_count = 0
    start = time.time()

    while True:
        ret, frame = cap.read()
        if not ret:
            continue

        # Copy the latest frame into shared memory
        frame_array[:] = frame[:FRAME_H, :FRAME_W, :]

        # 시각화 (visualization)
        cv2.imshow("Camera View", frame[:FRAME_H, :FRAME_W, :])
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

        frame_count += 1

        # Print FPS every 1 second
        if time.time() - start >= 1.0:
            print(f"[Camera] FPS={frame_count}")
            frame_count = 0
            start = time.time()

        # Small sleep to reduce CPU usage
        time.sleep(0.001)

    # 자원 해제
    cap.release()
    cv2.destroyAllWindows()
    shm.close()
