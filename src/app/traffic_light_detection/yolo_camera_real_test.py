# to visualize : uncomment cv2.imshow("Traffic Light Detection", frame)

import cv2
import numpy as np
import onnxruntime as ort

# =====================================================
# 1) Load ONNX model
# =====================================================
session = ort.InferenceSession(
    "best.onnx",
    providers=["CPUExecutionProvider"]
)
input_name = session.get_inputs()[0].name

# =====================================================
# 2) Class definitions (data.yaml based)
# =====================================================
CLASS_NAMES = {
    0: "GREEN",
    1: "RED",
    2: "YELLOW"
}

# Output mapping
# GREEN -> 3, RED -> 1, YELLOW -> 2
CLASS_MAP = {
    0: 3,
    1: 1,
    2: 2
}

COLORS = {
    0: (0, 255, 0),     # GREEN
    1: (0, 0, 255),     # RED
    2: (0, 255, 255)   # YELLOW
}

# =====================================================
# 3) Preprocess
# =====================================================
def preprocess(frame):
    img = cv2.resize(frame, (640, 640))
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = img.astype(np.float32) / 255.0
    img = np.transpose(img, (2, 0, 1))
    return np.expand_dims(img, axis=0)

# =====================================================
# 4) Convert xywh -> xyxy
# =====================================================
def xywh_to_xyxy(x, y, w, h, img_w, img_h):
    x1 = int((x - w / 2) * img_w)
    y1 = int((y - h / 2) * img_h)
    x2 = int((x + w / 2) * img_w)
    y2 = int((y + h / 2) * img_h)
    return x1, y1, x2, y2

# =====================================================
# 5) Camera
# =====================================================
cap = cv2.VideoCapture(0)
CONF_TH = 0.25

print("=== Traffic Light Detection Started (ESC to quit) ===")

# =====================================================
# 6) Main loop
# =====================================================
while True:
    ret, frame = cap.read()
    if not ret:
        print("ERROR: Failed to read camera frame")
        break

    h, w, _ = frame.shape

    # -----------------------------
    # Inference
    # -----------------------------
    input_tensor = preprocess(frame)
    outputs = session.run(None, {input_name: input_tensor})

    output = np.squeeze(outputs[0])   # (7, 8400)

    boxes = output[:4, :]
    scores = output[4:, :]

    class_ids = np.argmax(scores, axis=0)
    confidences = np.max(scores, axis=0)

    best_idx = np.argmax(confidences)
    best_conf = confidences[best_idx]
    best_class = class_ids[best_idx]

    # -----------------------------
    # Visualization
    # -----------------------------
    if best_conf > CONF_TH:
        x, y, bw, bh = boxes[:, best_idx]
        x1, y1, x2, y2 = xywh_to_xyxy(x, y, bw, bh, w, h)

        color = COLORS[best_class]
        label = f"{CLASS_NAMES[best_class]} {best_conf:.2f}"

        cv2.rectangle(frame, (x1, y1), (x2, y2), color, 3)
        cv2.putText(
            frame,
            label,
            (x1, y1 - 10),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            color,
            2
        )

        print(f"DETECTED: {CLASS_NAMES[best_class]} -> {CLASS_MAP[best_class]}")

    else:
        print("NOT DETECTED")

    # -----------------------------
    # Display
    # -----------------------------
    # cv2.imshow("Traffic Light Detection", frame)

    if cv2.waitKey(1) & 0xFF == 27:
        break

# =====================================================
# 7) Cleanup
# =====================================================
cap.release()
cv2.destroyAllWindows()
