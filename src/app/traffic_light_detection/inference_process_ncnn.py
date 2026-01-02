import cv2
import numpy as np
import time
from multiprocessing import shared_memory
import posix_ipc
import struct
import ncnn

# =========================
# POSIX Message Queue specification
# =========================
MQ_NAME = "/traffic_sign_state"

# traffic_sign_state definition
# 0: NONE, 1: RED, 2: YELLOW, 3: GREEN
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
    # Note: main.py uses 640x480
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
        max_message_size=4
    )

    print(f"[Inference NCNN] POSIX MQ ready: {MQ_NAME}")

    # =========================
    # NCNN Model Initialization (Direct Usage)
    # =========================
    net = ncnn.Net()
    # Load optimized model params and bin
    # net.load_param("best_ncnn_model/model.ncnn.param")
    # net.load_model("best_ncnn_model/model.ncnn.bin")
    net.load_param("yolov8n_ncnn_model/model.ncnn.param")
    net.load_model("yolov8n_ncnn_model/model.ncnn.bin")
    
    # Optimization settings
    net.opt.use_vulkan_compute = False # CPU only
    net.opt.num_threads = 4            # Use 4 cores
    
    # Enable ARM NEON optimization (FP16/INT8)
    net.opt.use_fp16_packed = True
    net.opt.use_fp16_storage = True
    net.opt.use_fp16_arithmetic = True
    net.opt.use_int8_inference = True

    print("[Inference NCNN] Model loaded successfully")

    target_size = 320
    # YOLO normalization: 0~255 -> 0~1
    norm_vals = [1/255.0, 1/255.0, 1/255.0]
    mean_vals = [] 

    while True:
        # Copy frame from shared memory
        frame = frame_array.copy()
        start = time.perf_counter()

        # =========================
        # NCNN Preprocessing
        # =========================
        # Resize and Normalize directly using NCNN's optimized function
        # ncnn.Mat.from_pixels_resize(data, type, w, h, target_w, target_h)
        # PIXEL_BGR2RGB = 2
        in_mat = ncnn.Mat.from_pixels_resize(
            frame, 
            ncnn.Mat.PixelType.PIXEL_BGR2RGB, 
            320, 240, 
            target_size, target_size
        )
        in_mat.substract_mean_normalize(mean_vals, norm_vals)

        # =========================
        # NCNN Inference
        # =========================
        with net.create_extractor() as ex:
            ex.input("in0", in_mat)
            ret, out_mat = ex.extract("out0")
            
            # Convert NCNN Mat to Numpy
            # output shape: (1, 7, 4725) or (7, 4725)
            output = np.array(out_mat)

        # =========================
        # Post-Processing
        # =========================
        # output is typically (7, 4725) -> [cx, cy, w, h, class0, class1, class2]
        
        # Extract class confidence scores
        # output[4:, :] -> class scores (3 rows, 4725 cols)
        scores = output[4:, :] 
        
        # Find best detection
        if scores.size > 0:
            best_idx = np.argmax(scores.max(axis=0))
            best_conf = scores[:, best_idx].max()
            best_class = np.argmax(scores[:, best_idx])
        else:
            best_conf = 0.0
            best_class = 0

        # =========================
        # Determine traffic_sign_state
        # =========================
        traffic_sign_state = 0 # Default: NONE
        
        if best_conf > CONF_TH:
            traffic_sign_state = CLASS_MAP.get(best_class, 0)
            
            # --- Visualization ---
            cx = output[0, best_idx]
            cy = output[1, best_idx]
            w = output[2, best_idx]
            h = output[3, best_idx]

            # Scale coordinates back to 640x480
            scale_x = 640 / target_size
            scale_y = 480 / target_size

            x1 = int((cx - w/2) * scale_x)
            y1 = int((cy - h/2) * scale_y)
            x2 = int((cx + w/2) * scale_x)
            y2 = int((cy + h/2) * scale_y)

            color = COLOR_MAP.get(best_class, (255, 255, 255))
            cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
            
            label = f"{LABEL_MAP.get(best_class, 'Unknown')} {best_conf:.2f}"
            cv2.putText(frame, label, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)

        # =========================
        # Send result via POSIX Message Queue
        # =========================
        mq.send(struct.pack("i", traffic_sign_state))

        infer_ms = (time.perf_counter() - start) * 1000

        # Draw inference time
        cv2.putText(frame, f"NCNN: {infer_ms:.1f}ms", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

        # Show Result
        cv2.imshow("NCNN Result", frame)
        if cv2.waitKey(1) == ord('q'):
            break

        print(
            f"[MQ SEND] traffic_sign_state={traffic_sign_state} "
            f"(conf={best_conf:.2f}, {infer_ms:.1f}ms)"
        )
