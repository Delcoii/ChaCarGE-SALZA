실제 절차별 다운로드 → 라즈베리파이 업그레이드 → 환경구성(파이썬) → ONIX 구현 환경 설정 → best.pt로 실행 동작

1. 라즈베리파이 OS 업데이트

```jsx
sudo apt update && sudo apt upgrade -y
```

- **왜 필요한가?**
최신 커널과 드라이버를 적용해야 USB 웹캠, Python 패키지, 네트워크 등이 안정적으로 동작합니다. 오래된 OS에서는 카메라 인식 오류가 발생할 수 있습니다.

2. 필수 패키지 설치

```jsx
sudo apt install python3-pip python3-opencv git -y
```

- **왜 필요한가?**
- `pip`: Python 패키지 설치 관리
- `opencv-python`: 웹캠 영상 캡처 및 이미지 처리
- `git`: YOLO 라이브러리 다운로드 및 관리

3. 딥러닝 환경 준비

```jsx
pip install ultralytics onnxruntime
```

- **왜 필요한가?**
- `ultralytics`: YOLOv8 프레임워크 (모델 불러오기, 추론 실행)
- `onnxruntime`: ONNX 모델 최적화 실행 엔진 (라즈베리파이 CPU에서 속도 ↑)

학습한 `best.pt`를 그대로 쓸 수도 있지만, **ONNX 변환 후 실행**이 Pi4에서는 훨씬 빠릅니다.

⇒ 만약 YOLO 정책으로 인해 직접 설치가 불가하다면? → 가상환경으로 설치하기

**1)venv 패키지 설치 확인**

```jsx
sudo apt install python3-venv python3-full -y
```

- `venv`: 가상환경 생성용
- `python3-full`: Python 표준 라이브러리 전체 포함

**2)가상환경 생성**

```jsx
python3 -m venv yolovenv
```

- `yolovenv`라는 폴더에 독립된 Python 환경 생성

**3)가상환경 활성화**

```jsx
source yolovenv/bin/activate
```

- 프롬프트 앞에 `(yolovenv)` 표시가 나오면 성공

**4)YOLO 및 의존성 설치**

```jsx
pip install --upgrade pip
pip install ultralytics onnxruntime opencv-python
```

- 이제 가상환경 안에서 YOLOv11n, ONNX Runtime, OpenCV 설치 가능

4. 웹캠 연결 확인

```jsx
ls /dev/video*
```

- **왜 필요한가?**
USB 웹캠이 정상적으로 인식되었는지 확인합니다. 보통 `/dev/video0`로 잡힙니다.
→ 만약 안 잡히면 드라이버 문제이므로 OS 업데이트/재부팅 필요.

5. 모델 파일 준비

- Colab/T4에서 학습한 `best.pt`를 라즈베리파이에 복사합니다.
- 변환 (추천):

```jsx
yolo export model=best.pt format=onnx
```

- **왜 필요한가?**
- `best.pt`: PyTorch 모델 → Pi4에서 실행 가능하지만 느림
- `best.onnx`: ONNX 모델 → Pi4 CPU에서 최적화된 속도로 실행 가능 → 해당 선택

6. 테스트 코드 작성

```jsx
from ultralytics import YOLO

# best.pt 직접 실행 (속도 느림)
model = YOLO("best.pt")
model.predict(source=0, show=True, imgsz=320, conf=0.5)

또는 ONNX Runtime 사용:

import cv2, onnxruntime as ort, numpy as np

cap = cv2.VideoCapture(0)  # USB 웹캠
session = ort.InferenceSession("best.onnx", providers=["CPUExecutionProvider"])

while True:
    ret, frame = cap.read()
    if not ret:
        continue
    img = cv2.resize(frame, (320, 320))
    x = img[:, :, ::-1].astype(np.float32) / 255.0
    x = np.transpose(x, (2, 0, 1))[None, ...]
    outputs = session.run(None, {"images": x})
    # TODO: postprocess → red/yellow/green 상태 추출
    cv2.imshow("Traffic Light Detection", img)
    if cv2.waitKey(1) == 27:  # ESC 종료
        break
```

- **왜 필요한가?**
- `source=0`: USB 웹캠 입력
- `imgsz=320`: Pi4에서 속도 확보
- `conf=0.5`: 신호등 확실히 잡도록 confidence threshold 설정

🚀 최종 정리

1. OS 업데이트 → 최신 드라이버 확보
2. Python/OpenCV 설치 → 카메라 영상 처리 가능
3. YOLO/ONNX 설치 → 모델 추론 가능
4. 웹캠 연결 확인 → `/dev/video0` 장치 확인
5. 모델 파일 준비 → `best.pt` 또는 변환된 `best.onnx`
6. 테스트 코드 실행 → 실시간 신호등 인식

해당까지 완료되었다면 이제부터 이를 통해 ONNX로 변환 이후 계속해서 진행 → 여기까지가 라즈베리파이4  환경구성!!
