**라즈베리파이4 best.pt -> best.onnx 변환 후 test**

flow(아래 대로 실행)

- 프로젝트 폴더 생성
```bash
mkdir yolo_export
cd yolo_export
```
- 가상환경 생성
```
python -m venv venv
```
- 가상환경 활성화
```
source venv/bin/activate
```
-필수 패키지 설치
```
pip install --upgrade pip
pip install ultralytics onnx onnxsim
```
- pt -> onnx 변환( 640 사이즈 변환)
```
yolo export model=best.pt format=onnx imgsz=640 opset=12 simplify=True
```
=> 이후 실행
