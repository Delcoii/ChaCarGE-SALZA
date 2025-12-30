import sys
import time
import mmap
import ctypes
import os
import numpy as np
import csv
import threading
import datetime
from collections import deque

from PyQt6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget
from PyQt6.QtCore import QTimer
import pyqtgraph as pg

# =========================================================
# 1. C Structure define
# =========================================================
class IMUAccel(ctypes.Structure):
    _fields_ = [("x_mps2", ctypes.c_double), ("y_mps2", ctypes.c_double), ("z_mps2", ctypes.c_double)]

class TrafficSignState(ctypes.Structure):
    _fields_ = [("data", ctypes.c_uint32)]

class VehicleCommand(ctypes.Structure):
    _fields_ = [("throttle", ctypes.c_double), ("brake", ctypes.c_double), ("steer_tire_degree", ctypes.c_double)]

class DriveDistance(ctypes.Structure):
    _fields_ = [("data_km", ctypes.c_double)]

class ShmGivenInfo(ctypes.Structure):
    _fields_ = [
        ("imu_accel", IMUAccel),
        ("traffic_state", TrafficSignState),
        ("vehicle_command", VehicleCommand),
        ("drive_distance", DriveDistance)
    ]

# (Declared to reserve space, though logically optional)
class DrivingScore(ctypes.Structure):
    _fields_ = [("total_score", ctypes.c_double), ("score_type", ctypes.c_int32)]
class ShmGeneratedInfo(ctypes.Structure):
    _fields_ = [("driving_score", DrivingScore)]
class ShmIntegrated(ctypes.Structure):
    _fields_ = [("given_info", ShmGivenInfo), ("generated_info", ShmGeneratedInfo)]


# =========================================================
# data logging thread class
# =========================================================
class DataLogger(threading.Thread):
    def __init__(self, shm_data):
        super().__init__()
        self.shm_data = shm_data
        self.running = True
        
        # file name (ex: log_20240520_143000.csv)
        now = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        self.filename = f"log_{now}.csv"
        print(f"[Logger] Recording started: {self.filename}")

    def run(self):
        # open CSV file
        with open(self.filename, mode='w', newline='') as file:
            writer = csv.writer(file)
            
            # header(columns)
            writer.writerow(['Timestamp_sec', 'Throttle', 'Brake', 'Steer', 
                             'AccX', 'AccY', 'AccZ', 'Distance_km', 'TrafficSignal'])
            
            start_time = time.time()

            while self.running:
                # 1. Read Data from Shared Memory
                info = self.shm_data.given_info
                
                # 2. Calculate Relative Time
                elapsed_time = time.time() - start_time
                
                # 3. CSV Write
                writer.writerow([
                    f"{elapsed_time:.3f}",                  # 0.000s format
                    f"{info.vehicle_command.throttle:.1f}",
                    f"{info.vehicle_command.brake:.1f}",
                    f"{info.vehicle_command.steer_tire_degree:.2f}",
                    f"{info.imu_accel.x_mps2:.3f}",
                    f"{info.imu_accel.y_mps2:.3f}",
                    f"{info.imu_accel.z_mps2:.3f}",
                    f"{info.drive_distance.data_km:.5f}",
                    f"{info.traffic_state.data}"
                ])
                
                # 4. Periodic wait (10ms = 0.01sec = 100Hz)
                # Note: Python sleep is not highly precise, but sufficient for logging purposes
                time.sleep(0.01) 

    def stop(self):
        self.running = False
        print(f"[Logger] Recording saved to {self.filename}")


# =========================================================
# 2. Real time graph class (PlotJuggler style)
# =========================================================
class RealTimePlotter(QMainWindow):
    def __init__(self):
        super().__init__()
        
        # shared memory setup
        self.SHM_NAME = "/dev/shm/shm_integrated"
        if not os.path.exists(self.SHM_NAME):
            print("Error: Generator를 먼저 실행해주세요!")
            sys.exit(1)
            
        self.f = open(self.SHM_NAME, "r+b")
        self.mm = mmap.mmap(self.f.fileno(), ctypes.sizeof(ShmIntegrated))
        self.shm_data = ShmIntegrated.from_buffer(self.mm)

        # window setup
        self.setWindowTitle("Mini PlotJuggler (Python Edition)")
        self.resize(1000, 800)

        # main widget and layout
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        self.layout = QVBoxLayout(self.central_widget)

        # --- graph config (black background) ---
        pg.setConfigOption('background', 'k')
        pg.setConfigOption('foreground', 'w')

        # 1. Vehicle Command Graph (Throttle & Brake)
        self.plot_cmd = pg.PlotWidget(title="Vehicle Command")
        self.plot_cmd.addLegend()
        self.plot_cmd.setYRange(-5, 105)
        self.curve_throttle = self.plot_cmd.plot(pen=pg.mkPen('g', width=2), name="Throttle")
        self.curve_brake = self.plot_cmd.plot(pen=pg.mkPen('r', width=2), name="Brake")
        self.layout.addWidget(self.plot_cmd)

        # 2. Steering Graph
        self.plot_steer = pg.PlotWidget(title="Steering Angle")
        self.plot_steer.setYRange(-30, 30)
        self.curve_steer = self.plot_steer.plot(pen=pg.mkPen('c', width=2), name="Steer")
        # Zero line
        self.plot_steer.addItem(pg.InfiniteLine(pos=0, angle=0, pen=pg.mkPen('w', style=pg.QtCore.Qt.PenStyle.DashLine)))
        self.layout.addWidget(self.plot_steer)

        # 3. IMU graph
        self.plot_imu = pg.PlotWidget(title="IMU Acceleration")
        self.plot_imu.addLegend()
        self.curve_imu_x = self.plot_imu.plot(pen=pg.mkPen('y', width=2), name="Acc X")
        self.curve_imu_y = self.plot_imu.plot(pen=pg.mkPen('b', width=2), name="Acc Y")
        self.curve_imu_z = self.plot_imu.plot(pen=pg.mkPen('m', width=2), name="Acc Z")
        self.layout.addWidget(self.plot_imu)

        # data buffer (present 300 dots save)
        self.buffer_size = 300
        
        self.data_throttle = deque([0]*self.buffer_size, maxlen=self.buffer_size)
        self.data_brake = deque([0]*self.buffer_size, maxlen=self.buffer_size)
        self.data_steer = deque([0]*self.buffer_size, maxlen=self.buffer_size)
        self.data_imu_x = deque([0]*self.buffer_size, maxlen=self.buffer_size)
        self.data_imu_y = deque([0]*self.buffer_size, maxlen=self.buffer_size)
        self.data_imu_z = deque([0]*self.buffer_size, maxlen=self.buffer_size)

        # Timer maintained for display refresh (30ms)
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(30)

        # logging thread start (10ms period)
        self.logger = DataLogger(self.shm_data)
        self.logger.start()

    def update_plot(self):
        # 1. read data from shared memory (Raw Access)
        info = self.shm_data.given_info
        
        # 2. add data to buffer
        self.data_throttle.append(info.vehicle_command.throttle)
        self.data_brake.append(info.vehicle_command.brake)
        self.data_steer.append(info.vehicle_command.steer_tire_degree)
        self.data_imu_x.append(info.imu_accel.x_mps2)
        self.data_imu_y.append(info.imu_accel.y_mps2)
        self.data_imu_z.append(info.imu_accel.z_mps2)

        # 3. graph update
        self.curve_throttle.setData(self.data_throttle)
        self.curve_brake.setData(self.data_brake)
        self.curve_steer.setData(self.data_steer)
        self.curve_imu_x.setData(self.data_imu_x)
        self.curve_imu_y.setData(self.data_imu_y)
        self.curve_imu_z.setData(self.data_imu_z)

    def closeEvent(self, event):
        # logging thread stop when window close
        if self.logger.is_alive():
            self.logger.stop()
            self.logger.join() # wait for thread to completely finish
        
        # file and mmap close
        self.mm.close()
        self.f.close()
        event.accept()

# =========================================================
# main execution
# =========================================================
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = RealTimePlotter()
    window.show()
    sys.exit(app.exec())