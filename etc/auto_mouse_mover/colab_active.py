import ctypes
import time
import sys

# 윈도우 API 구조체 정의
class POINT(ctypes.Structure):
    _fields_ = [("x", ctypes.c_long), ("y", ctypes.c_long)]

def main():
    print("========================================")
    print("   Colab Keep-Alive (Python Version)")
    print("   Move/Scroll every 5 seconds.")
    print("   Press Ctrl+C to stop.")
    print("========================================")

    # 윈도우 API 로드
    user32 = ctypes.windll.user32
    
    # 마우스 이벤트 상수
    MOUSEEVENTF_WHEEL = 0x0800

    point = POINT()
    
    try:
        while True:
            # 1. 현재 커서 위치 가져오기
            user32.GetCursorPos(ctypes.byref(point))
            current_x = point.x
            current_y = point.y

            # 2. 마우스 미세 이동 (오른쪽 1픽셀 -> 복귀)
            user32.SetCursorPos(current_x + 10, current_y + 10)
            time.sleep(0.01)
            user32.SetCursorPos(current_x, current_y)

            # 3. 마우스 휠 스크롤 (살짝 위로)
            # mouse_event(dwFlags, dx, dy, dwData, dwExtraInfo)
            user32.mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 100, 0)
            time.sleep(0.1)
            user32.mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -100, 0)

            timestamp = time.strftime("%H:%M:%S")
            print(f"[{timestamp}] Active check: Mouse moved & Scrolled")

            # 4. 60초 대기
            time.sleep(5)

    except KeyboardInterrupt:
        print("\nProgram stopped by user.")

if __name__ == "__main__":
    main()

