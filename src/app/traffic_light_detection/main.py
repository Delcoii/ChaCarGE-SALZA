from multiprocessing import shared_memory, Process
from camera_process import camera_process
from inference_process import inference_process
# from inference_process_ncnn import inference_process
# from inference_process_pt import inference_process
from can_process import can_process

FRAME_W, FRAME_H, FRAME_C = 320, 240, 3
FRAME_SIZE = FRAME_W * FRAME_H * FRAME_C

if __name__ == "__main__":
    # Create shared memory for camera frames
    shm = shared_memory.SharedMemory(create=True, size=FRAME_SIZE)

    # Create processes
    p_cam = Process(
        target=camera_process,
        args=(shm.name,)
    )

    p_inf = Process(
        target=inference_process,
        args=(shm.name,)      # Pass only shared memory name
    )

    p_con = Process(
        target=can_process    # No arguments needed
    )

    # Start processes
    p_cam.start()
    p_inf.start()
    p_con.start()

    # Wait for processes to finish
    p_cam.join()
    p_inf.join()
    p_con.join()

    # Cleanup shared memory
    shm.close()
    shm.unlink()
