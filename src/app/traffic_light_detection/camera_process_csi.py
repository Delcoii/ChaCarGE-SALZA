import time
import numpy as np
from multiprocessing import shared_memory
from picamera2 import Picamera2

# Keep the frame dimensions consistent with the main process
FRAME_W, FRAME_H, FRAME_C = 320, 240, 3

def camera_process(shm_name):
    # 1. Initialize Picamera2
    picam2 = Picamera2()
    
    # 2. Configure the camera (similar to cap.set for USB cameras)
    # Since the model input is 320x240, capture frames directly at this size.
    # Set display=None to avoid GUI errors that may occur otherwise.
    config = picam2.create_preview_configuration(
        main={"format": "RGB888", "size": (FRAME_W, FRAME_H)},
        display=None
    )
    picam2.configure(config)
    picam2.start()

    # 3. Connect to shared memory
    shm = shared_memory.SharedMemory(name=shm_name)
    frame_array = np.ndarray(
        (FRAME_H, FRAME_W, FRAME_C),
        dtype=np.uint8,
        buffer=shm.buf
    )

    frame_count = 0
    start = time.time()

    print("[Camera] Picamera2 (CSI) Process Started")

    try:
        while True:
            # 4. Capture a frame from the CSI camera (RGB array directly)
            frame = picam2.capture_array()

            # 5. Copy the frame into shared memory
            # Picamera2 outputs RGB888 by default.
            # If the inference process expects BGR, convert as shown below:
            # frame_array[:] = frame[:, :, ::-1]  # RGB to BGR
            frame_array[:] = frame

            frame_count += 1

            # Print FPS every second
            if time.time() - start >= 1.0:
                print(f"[Camera] FPS={frame_count}")
                frame_count = 0
                start = time.time()

            # CSI cameras are hardware-synchronized,
            # so only a very short sleep is needed to reduce CPU load.
            time.sleep(0.001)

    except Exception as e:
        print(f"[Camera] Error: {e}")
    finally:
        picam2.stop()
        shm.close()
        print("[Camera] Stopped")
