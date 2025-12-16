from multiprocessing import shared_memory, Process, Queue
from camera_process import camera_process
from inference_process import inference_process
from can_process import can_process

FRAME_W, FRAME_H, FRAME_C = 320, 240, 3
FRAME_SIZE = FRAME_W * FRAME_H * FRAME_C

if __name__ == "__main__":
    shm = shared_memory.SharedMemory(create=True, size=FRAME_SIZE)
    msg_queue = Queue()

    p_cam = Process(target=camera_process, args=(shm.name,))
    p_inf = Process(target=inference_process, args=(shm.name, msg_queue))
    p_con = Process(target=can_process, args=(msg_queue,))

    p_cam.start()
    p_inf.start()
    p_con.start()

    p_cam.join()
    p_inf.join()
    p_con.join()

    shm.close()
    shm.unlink()