```
!pip install ultralytics
```
#Colab에서 YOLOv8 설치:

```
from google.colab import files
uploaded = files.upload()   
```
# 여기서 zip 파일 선택 -> 직접내가 올릴  .zip 파일 올리기


```
import os
os.listdir('/content') 
```
# 실제 내 zip 파일명 다시 확인

```
import zipfile

zip_path = "/content/Traffic light reconizetion.v4i.yolov8.zip" 
with zipfile.ZipFile(zip_path, 'r') as zip_ref:
zip_ref.extractall("/content/dataset")
```
# -> 해당 .zip파일 압축 푸는 과정

```
import os
os.listdir('/content/dataset') 
```
#내가 압축해제한 dataset 확인 (data.yaml 잘 보고 요소 확인)

내 data.yaml 요소 확인
```
train: ../train/images
val: ../valid/images
test: ../test/images

nc: 3
names: ['Green', 'Red', 'Yellow'] # 실제 내가 분류할 이름 3가지

roboflow:
  workspace: jeonjin
  project: traffic-light-rmaje-cdntq
  version: 1
  license: CC BY 4.0
  url: https://universe.roboflow.com/jeonjin/traffic-light-rmaje-cdntq/dataset/1
```

```
import os
os.listdir('/content/dataset') # yaml 위치 확인
```

#실제 최종 데이터 시트를 가지고 교육돌리기(가상 GPU이용 반드시 장시간 소요)
```
!yolo detect train \
  data=/content/dataset/data.yaml \
  model=yolov8n.pt \
  epochs=2 \
  imgsz=960 \
  batch=16 \
  patience=50 \
  optimizer=AdamW \
  lr0=0.002 \
  lrf=0.01 \
  weight_decay=0.0005 \
  augment=True \
  hsv_h=0.015 \
  hsv_s=0.7 \
  hsv_v=0.4 \
  degrees=10 \
  translate=0.1 \
  scale=0.5 \
  shear=2 \
  perspective=0.0005 \
  flipud=0.0 \
  fliplr=0.5 \
  mosaic=1.0 \
  mixup=0.2 \
  project=/content/runs \
  name=traffic_yolov8n_opt
```

-> 이 과정으로 내가 가진 data set을 yolo v8n (경량화) 모델에 맞게 교육시켜 최적, 최고의 best.pt 생성하기

데이터 관련
- data=/content/dataset/data.yaml
→ 학습에 사용할 데이터셋 경로와 클래스 정의(red, yellow, green)를 지정합니다.
- model=yolov8n.pt
→ YOLOv8n(초경량 모델) 프리트레인 가중치를 불러와서 transfer learning 시작. 작은 객체 탐지에 적합하고 라즈베리파이에서도 빠르게 동작합니다.

⚙️ 학습 설정
- epochs=200
→ 학습 반복 횟수. 신호등처럼 작은 객체는 충분히 학습해야 하므로 200으로 늘려 안정적 수렴을 유도합니다.
- imgsz=960
→ 입력 이미지 크기. 해상도를 크게 해서 작은 신호등 객체를 더 잘 인식할 수 있게 합니다. 추론 시에는 320~416으로 줄여서 속도를 확보합니다.
- batch=32
→ 한 번에 학습하는 이미지 수. GPU 메모리와 속도 균형을 맞춘 값입니다.
- patience=50
→ Early stopping 기준. 50 epoch 동안 성능 향상이 없으면 학습을 멈춰 과적합 방지.

🧠 최적화 관련
- optimizer=AdamW
→ Adam보다 일반화 성능이 좋은 옵티마이저. 작은 객체 탐지에서 안정적인 학습을 돕습니다.
- lr0=0.002
→ 초기 학습률. 조금 높게 시작해 빠른 학습을 유도합니다.
- lrf=0.01
→ 최종 학습률 비율. Cosine decay로 학습률을 점점 줄여 안정적 수렴을 유도합니다.
- weight_decay=0.0005
→ 과적합 방지용 규제. 불필요한 파라미터 업데이트를 줄여 모델을 더 일반화합니다.

🎨 데이터 증강 (Augmentation)
- augment=True
→ 데이터 증강 활성화. 다양한 조건에서 robust한 모델을 만듭니다.
- hsv_h=0.015, hsv_s=0.7, hsv_v=0.4
→ 색상/채도/밝기 변형. 조명, 날씨, 카메라 노출 변화에도 신호등 색을 잘 인식하게 합니다.
- degrees=10
→ 회전 변형. 카메라 각도 변화에 대응.
- translate=0.1
→ 위치 이동. 신호등이 화면 내 다양한 위치에 있어도 인식 가능.
- scale=0.5
→ 크기 변형. 멀리 있는 작은 신호등도 학습.
- shear=2
→ 기울기 변형. 비스듬히 찍힌 신호등에도 대응.
- perspective=0.0005
→ 원근 변형. 카메라 각도 차이에 대응.
- flipud=0.0, fliplr=0.5
→ 상하 반전은 없음, 좌우 반전은 50% 확률. 다양한 시야 확보.
- mosaic=1.0
→ 여러 이미지를 합쳐 작은 객체 인식 강화. 신호등 탐지에 매우 효과적.
- mixup=0.2
→ 두 이미지를 섞어 학습. 일반화 성능 강화.

📁 결과 저장
- project=/content/runs
→ 학습 결과 저장 위치.
- name=traffic_yolov8n_opt
→ 실험 이름. 결과 폴더를 구분하기 쉽게 합니다.
