#위에서 부터 아래로 모든 실행 시 바로 실행 가능 합니다.
   
### 시스템 업데이트
sudo apt update && sudo apt upgrade -y

### 필수 빌드 도구
sudo apt install -y build-essential cmake pkg-config git

### Python3 및 venv
sudo apt install -y python3 python3-venv python3-pip

2. 가상 환경 생성 및 활성화
### 프로젝트 폴더 이동
cd ~/yolo_camera

### 가상환경 생성
python3 -m venv yolo_env

### 활성화
source yolo_env/bin/activate

3. YOLO 추론 관련 패키지
### OpenCV
pip install opencv-python

### Numpy
pip install numpy

### ONNX Runtime (CPU 버전)
pip install onnxruntime

4. POSIX IPC(Message Queue)
### posix_ipc 설치
pip install posix_ipc

5. CAN 통신 (Python-can)
### python-can 설치
pip install python-can

6. Shared Memory
### Python 표준 라이브러리 multiprocessing.shared_memory는 별도 설치 필요 없음 (Python 3.8+ 내장).

7. socketCAN 커널 모듈
### CAN 커널 모듈 로드
sudo modprobe can
sudo modprobe can_raw
sudo modprobe vcan   # 가상 CAN 테스트용

8. CAN 인터페이스 설정
###실제 MCP2515 모듈 사용 시 /boot/firmware/config.txt에 오버레이 추가:
=> sudo nano 해서 들어가서 맨 아래에 아래 두줄 추가 후 저장
dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=25
dtoverlay=spi-bcm2835

=> 이후 
sudo reboot 

9. exec
가상 환경 상태 들어간 상태(앞에 가상 환경 명 들어가 있음)
python main.py 
