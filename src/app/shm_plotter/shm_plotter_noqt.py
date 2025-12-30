import sys
import time
import mmap
import ctypes
import os
import csv
import datetime
import signal

# =========================================================
# 1. C-style Struct Definitions (Same as existing ones)
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

# (Trailing structs can be omitted but declared for size calculation)
class DrivingScore(ctypes.Structure):
    _fields_ = [("total_score", ctypes.c_double), ("score_type", ctypes.c_int32)]
class ShmGeneratedInfo(ctypes.Structure):
    _fields_ = [("driving_score", DrivingScore)]
class ShmIntegrated(ctypes.Structure):
    _fields_ = [("given_info", ShmGivenInfo), ("generated_info", ShmGeneratedInfo)]

# =========================================================
# 2. Main Logging System
# =========================================================
def main():
    # Shared memory file path
    SHM_NAME = "/dev/shm/shm_integrated"
    
    # 1. Check if Generator is running
    if not os.path.exists(SHM_NAME):
        print(f"[Error] File {SHM_NAME} not found.")
        print(">> Please run the C Generator first.")
        return

    # 2. Connect to shared memory
    try:
        f = open(SHM_NAME, "r+b")
        mm = mmap.mmap(f.fileno(), ctypes.sizeof(ShmIntegrated))
        shm_data = ShmIntegrated.from_buffer(mm)
    except Exception as e:
        print(f"[Error] Failed to connect to shared memory: {e}")
        return

    # 3. Prepare CSV file
    now = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"log_{now}.csv"
    
    print(f"==================================================")
    print(f"   Headless Data Logger (No GUI)")
    print(f"   File: {filename}")
    print(f"   Press [Ctrl + C] to stop recording")
    print(f"==================================================")

    # 4. Data collection loop
    try:
        with open(filename, mode='w', newline='') as file:
            writer = csv.writer(file)
            # Write header
            writer.writerow(['Timestamp_sec', 'Throttle', 'Brake', 'Steer', 
                             'AccX', 'AccY', 'AccZ', 'Distance_km', 'TrafficSignal'])
            
            start_time = time.time()
            frame_count = 0

            while True:
                # (1) Read data
                info = shm_data.given_info
                
                # (2) Calculate time
                current_time = time.time()
                elapsed_time = current_time - start_time
                
                # (3) Save to CSV
                writer.writerow([
                    f"{elapsed_time:.3f}",
                    f"{info.vehicle_command.throttle:.1f}",
                    f"{info.vehicle_command.brake:.1f}",
                    f"{info.vehicle_command.steer_tire_degree:.2f}",
                    f"{info.imu_accel.x_mps2:.3f}",
                    f"{info.imu_accel.y_mps2:.3f}",
                    f"{info.imu_accel.z_mps2:.3f}",
                    f"{info.drive_distance.data_km:.5f}",
                    f"{info.traffic_state.data}"
                ])
                
                # (4) Display status in terminal (Update every 100 frames = 1 second)
                frame_count += 1
                if frame_count % 100 == 0:
                    print(f"\r[Recording] Time: {elapsed_time:6.1f}s | Dist: {info.drive_distance.data_km:6.3f}km | Lines: {frame_count}", end="")

                # (5) Wait for period (10ms = 100Hz)
                time.sleep(0.01)

    except KeyboardInterrupt:
        print("\n\n[Stop] Termination request received.")
        print(f"[System] File saved successfully: {filename}")
    
    finally:
        # Resource cleanup
        mm.close()
        f.close()

if __name__ == "__main__":
    main()