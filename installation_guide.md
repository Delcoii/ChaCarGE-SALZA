#  Installation Guide
## Detection
### 1. Install Prerequisites
```bash
sudo apt update && sudo apt upgrade -y
sudo apt install python3-pip python3-opencv git -y
```

### 2. Virtual Environment Setting
```bash
sudo apt install python3-venv python3-full -y

# in home directory
mkdir yolo_export
cd yolo_export

python3 -m venv venv
source venv/bin/activate    # activating env
```

### 3. Install Python Library in virtual env
```bash
pip install --upgrade pip
pip install ultralytics onnxruntime opencv-python
pip install onnx onnxsim

# for communication
pip install posix-ipc
pip install python-can
```
### 4. Checking Camera Hardware (USB Camera)
```bash
ls /dev/video*
```
find `/dev/video0` (need to fix)

### 5. Preparing Model with ONNX conversion
```bash
# do it where best.pt is located
yolo export model=best.pt format=onnx imgsz=640 opset=12 simplify=True
```
*   `imgsz=640`: input image size
*   `simplify=True`: simplify ONNX model structure (using onnxsim)
*   `opset=12`: ONNX 호환 버전

---

### 6. Run Detection Model
```bash
# after activating virtual env
cd src/app/traffic_light_detection
python yolo_camera_real_test.py
```