실제 절차별 다운로드 → 라즈베리파이 업그레이드 → 환경구성(파이썬) → ONIX 구현 환경 설정 → best.pt로 실행 동작

**라즈베리파이4 Yolo 환경 구성**

```jsx
sudo apt update && sudo apt upgrade -y
sudo apt install python3-pip python3-opencv git -y
pip install ultralytics onnxruntime
```

**(환경 설정 불가시 -> 가상 환경) 패키지 설치 확인**

```jsx
sudo apt install python3-venv python3-full -y
python3 -m venv yolovenv
source yolovenv/bin/activate
pip install --upgrade pip
pip install ultralytics onnxruntime opencv-python
```

- 이제 가상환경 안에서 YOLOv11n, ONNX Runtime, OpenCV 설치 가능

웹캠 연결 확인
```jsx
ls /dev/video*
```

모델 파일 준비
```jsx
yolo export model=best.pt format=onnx
```

-테스트 코드 작성
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
-최종 정리

1. OS 업데이트 → 최신 드라이버 확보
2. Python/OpenCV 설치 → 카메라 영상 처리 가능
3. YOLO/ONNX 설치 → 모델 추론 가능
4. 웹캠 연결 확인 → `/dev/video0` 장치 확인
5. 모델 파일 준비 → `best.pt` 또는 변환된 `best.onnx`
6. 테스트 코드 실행 → 실시간 신호등 인식

해당까지 완료되었다면 이제부터 이를 통해 ONNX로 변환 이후 계속해서 진행 → 여기까지가 라즈베리파이4  환경구성!!
