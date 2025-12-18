# YOLO Camera Inference System on Raspberry Pi

This guide explains how to set up and run a YOLO-based traffic sign inference system on Raspberry Pi using Python, ONNX Runtime, Shared Memory, POSIX Message Queue, and CAN communication.

All commands can be executed sequentially from top to bottom.

---

## 1. Basic System Setup

### System Update
```
sudo apt update && sudo apt upgrade -y
```
###Install Essential Build Tools
```
sudo apt install -y build-essential cmake pkg-config git
```

###Install Python3 and Virtual Environment Tools
```
sudo apt install -y python3 python3-venv python3-pip
```

##2. Create and Activate Virtual Environment
###Move to Project Directory
```
cd ~/yolo_camera
```
###Create Virtual Environment
```
python3 -m venv yolo_env
```
###Activate Virtual Environment
```
source yolo_env/bin/activate
```

##3. YOLO Inference Dependencies
###OpenCV
```
pip install opencv-python
```
###NumPy
```
pip install numpy
```
###ONNX Runtime (CPU Version)
```
pip install onnxruntime
```

##4. POSIX IPC (Message Queue)
###Install posix_ipc
```
pip install posix_ipc
```

##5. CAN Communication (python-can)
###Install python-can
```
pip install python-can
```

##6. Shared Memory
Python’s multiprocessing.shared_memory module is included by default in Python 3.8 and later.
No additional installation is required.

##7. CAN Interface Configuration (MCP2515)
###Edit Boot Configuration
When using a real MCP2515 CAN module, add the following lines to the boot configuration file:

```
sudo nano /boot/firmware/config.txt
```

###Add These Lines at the End of the File
```
dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=25
dtoverlay=spi-bcm2835
```
###Reboot the System
```
sudo reboot
```

##9. Run the Application
###Activate Virtual Environment (if not already active)
```
source ~/yolo_camera/yolo_env/bin/activate
```
###Run the Main Program
```
python main.py
```

Notes

This project uses Shared Memory for frame transfer between processes.

POSIX Message Queue is used to transmit traffic sign state results.

ONNX Runtime (CPU) is used for YOLO inference.

CAN communication can be tested using vcan before connecting real hardware.
