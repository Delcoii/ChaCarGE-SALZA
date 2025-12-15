/* best.onnx 를 통해 실제 red->1 , yellow ->2 , green ->3 추론 하여 출력 하는 예제 코드 */

import cv2
import numpy as np
import onnxruntime as ort

# =====================================================
# 1️⃣ Load ONNX model
# =====================================================
session = ort.InferenceSession(
    "best.onnx",
    providers=["CPUExecutionProvider"]
)
input_name = session.get_inputs()[0].name

# =====================================================
# 2️⃣ Class mapping (based on data.yaml)
# 0 = green, 1 = red, 2 = yellow
# Output mapping:
#   green  -> 3
#   red    -> 1
#   yellow -> 2
# =====================================================
CLASS_MAP = {
    0: 3,  # green
    1: 1,  # red
    2: 2   # yellow
}

# =====================================================
# 3️⃣ Preprocess function (YOLOv8 ONNX format)
# =====================================================
def preprocess(img):
    # Resize to model input size
    img = cv2.resize(img, (640, 640))

    # Convert BGR to RGB
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    # Normalize to [0, 1]
    img = img.astype(np.float32) / 255.0

    # Change shape: (H, W, C) -> (C, H, W)
    img = np.transpose(img, (2, 0, 1))

    # Add batch dimension
    return np.expand_dims(img, axis=0)

# =====================================================
# 4️⃣ Open camera device
# =====================================================
cap = cv2.VideoCapture(0)

# Confidence threshold
CONF_TH = 0.25   # Lower threshold for Raspberry Pi

# =====================================================
# 5️⃣ Main loop
# =====================================================
while True:
    ret, frame = cap.read()
    if not ret:
        print("Cannot read camera frame, exiting")
        break

    # -------------------------------------------------
    # 5-1) Preprocess and run inference
    # -------------------------------------------------
    input_tensor = preprocess(frame)
    outputs = session.run(None, {input_name: input_tensor})

    # -------------------------------------------------
    # 5-2) Get ONNX output
    # Typical shape: (1, 7, 8400)
    # -------------------------------------------------
    output = outputs[0]
    output = np.squeeze(output)   # (7, 8400)

    # -------------------------------------------------
    # 5-3) Split class scores
    # 0~3 : bounding box (x, y, w, h)
    # 4~6 : class scores (green, red, yellow)
    # -------------------------------------------------
    scores = output[4:, :]        # (3, 8400)

    # -------------------------------------------------
    # 5-4) Select the highest confidence prediction
    # -------------------------------------------------
    class_ids = np.argmax(scores, axis=0)
    confidences = np.max(scores, axis=0)

    best_idx = np.argmax(confidences)
    best_conf = confidences[best_idx]
    best_class = class_ids[best_idx]

    # -------------------------------------------------
    # 5-5) Decision logic
    # -------------------------------------------------
    if best_conf > CONF_TH:
        signal = CLASS_MAP.get(best_class, None)
        if signal is not None:
            print(f"Detected signal value: {signal} (conf={best_conf:.2f})")
        else:
            print("Unknown class detected:", best_class)
    else:
        print("No traffic light detected (low confidence:", round(best_conf, 3), ")")

# =====================================================
# 6️⃣ Release resources
# =====================================================
cap.release()
