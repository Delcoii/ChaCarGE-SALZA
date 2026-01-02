import sys
import time
import mmap
import ctypes
import os
import numpy as np
from collections import deque

from PyQt6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget
from PyQt6.QtCore import QTimer
import pyqtgraph as pg

# =========================================================
# 1. C-style Struct Definitions (Must match C code)
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

# (Trailing structs for generated info)
class DrivingScore(ctypes.Structure):
    _fields_ = [("total_score", ctypes.c_double), ("score_type", ctypes.c_int32)]
class ShmGeneratedInfo(ctypes.Structure):
    _fields_ = [("driving_score", DrivingScore)]
class ShmIntegrated(ctypes.Structure):
    _fields_ = [("given_info", ShmGivenInfo), ("generated_info", ShmGeneratedInfo)]


# =========================================================
# 2. Real-time Plotter Class (Visualization Only)
# =========================================================
class RealTimePlotter(QMainWindow):
    def __init__(self):
        super().__init__()
        
        # Connect to Shared Memory
        self.SHM_NAME = "/dev/shm/shm_integrated"
        if not os.path.exists(self.SHM_NAME):
            print("Error: Please run the C Generator first!")
            sys.exit(1)
            
        self.f = open(self.SHM_NAME, "r+b")
        self.mm = mmap.mmap(self.f.fileno(), ctypes.sizeof(ShmIntegrated))
        self.shm_data = ShmIntegrated.from_buffer(self.mm)

        # Window Setup
        self.setWindowTitle("Real-time Data Visualizer")
        self.resize(1000, 800)

        # Main Widget and Layout
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        self.layout = QVBoxLayout(self.central_widget)

        # --- Graph Settings (Black Background for visibility) ---
        pg.setConfigOption('background', 'k')
        pg.setConfigOption('foreground', 'w')

        # Graph 1: Vehicle Command (Throttle & Brake)
        self.plot_cmd = pg.PlotWidget(title="Vehicle Command")
        self.plot_cmd.addLegend()
        self.plot_cmd.setYRange(-5, 105)
        self.curve_throttle = self.plot_cmd.plot(pen=pg.mkPen('g', width=2), name="Throttle")
        self.curve_brake = self.plot_cmd.plot(pen=pg.mkPen('r', width=2), name="Brake")
        self.layout.addWidget(self.plot_cmd)

        # Graph 2: Steering Angle
        self.plot_steer = pg.PlotWidget(title="Steering Angle")
        self.plot_steer.setYRange(-30, 30)
        self.curve_steer = self.plot_steer.plot(pen=pg.mkPen('c', width=2), name="Steer")
        # Add a reference line at 0 degrees
        self.plot_steer.addItem(pg.InfiniteLine(pos=0, angle=0, pen=pg.mkPen('w', style=pg.QtCore.Qt.PenStyle.DashLine)))
        self.layout.addWidget(self.plot_steer)

        # Graph 3: IMU Acceleration
        self.plot_imu = pg.PlotWidget(title="IMU Acceleration")
        self.plot_imu.addLegend()
        self.curve_imu_x = self.plot_imu.plot(pen=pg.mkPen('y', width=2), name="Acc X")
        self.curve_imu_y = self.plot_imu.plot(pen=pg.mkPen('b', width=2), name="Acc Y")
        self.curve_imu_z = self.plot_imu.plot(pen=pg.mkPen('m', width=2), name="Acc Z")
        self.layout.addWidget(self.plot_imu)

        # Data Buffers (Store recent 300 points for display)
        self.buffer_size = 300
        
        self.data_throttle = deque([0]*self.buffer_size, maxlen=self.buffer_size)
        self.data_brake = deque([0]*self.buffer_size, maxlen=self.buffer_size)
        self.data_steer = deque([0]*self.buffer_size, maxlen=self.buffer_size)
        self.data_imu_x = deque([0]*self.buffer_size, maxlen=self.buffer_size)
        self.data_imu_y = deque([0]*self.buffer_size, maxlen=self.buffer_size)
        self.data_imu_z = deque([0]*self.buffer_size, maxlen=self.buffer_size)

        # Setup Timer for Screen Refresh (30ms interval = approx 33 FPS)
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_plot)
        self.timer.start(30)

    def update_plot(self):
        # 1. Read data from shared memory (Raw Access)
        info = self.shm_data.given_info
        
        # 2. Append current values to buffers
        self.data_throttle.append(info.vehicle_command.throttle)
        self.data_brake.append(info.vehicle_command.brake)
        self.data_steer.append(info.vehicle_command.steer_tire_degree)
        self.data_imu_x.append(info.imu_accel.x_mps2)
        self.data_imu_y.append(info.imu_accel.y_mps2)
        self.data_imu_z.append(info.imu_accel.z_mps2)

        # 3. Update graph curves
        self.curve_throttle.setData(self.data_throttle)
        self.curve_brake.setData(self.data_brake)
        self.curve_steer.setData(self.data_steer)
        self.curve_imu_x.setData(self.data_imu_x)
        self.curve_imu_y.setData(self.data_imu_y)
        self.curve_imu_z.setData(self.data_imu_z)

    def closeEvent(self, event):
        # Cleanup shared memory mapping on exit
        self.mm.close()
        self.f.close()
        event.accept()

# =========================================================
# Main Execution
# =========================================================
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = RealTimePlotter()
    window.show()
    sys.exit(app.exec())