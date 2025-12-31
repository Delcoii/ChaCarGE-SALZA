import sys
import time
import mmap
import ctypes
import os
import csv
import argparse # Required for parsing command line arguments

# =========================================================
# 1. C-style Struct Definitions
#    (Must match the C header file exactly)
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

# (Trailing structs are optional for replay but kept for size alignment)
class DrivingScore(ctypes.Structure):
    _fields_ = [("total_score", ctypes.c_double), ("score_type", ctypes.c_int32)]
class ShmGeneratedInfo(ctypes.Structure):
    _fields_ = [("driving_score", DrivingScore)]
class ShmIntegrated(ctypes.Structure):
    _fields_ = [("given_info", ShmGivenInfo), ("generated_info", ShmGeneratedInfo)]

# =========================================================
# 2. Main Replay Logic
# =========================================================
def main():
    # Parse command line arguments (Get filename)
    parser = argparse.ArgumentParser(description="CSV Data Replayer")
    parser.add_argument("filename", help="Path to the CSV log file")
    args = parser.parse_args()

    # Check if file exists
    if not os.path.exists(args.filename):
        print(f"[Error] File '{args.filename}' not found.")
        return

    # Check Shared Memory Connection
    SHM_NAME = "/dev/shm/shm_integrated"
    if not os.path.exists(SHM_NAME):
        print("[Error] Shared Memory not found.")
        print(">> Please run the Generator first (even briefly) to create the memory.")
        return

    # Connect to Shared Memory
    try:
        f = open(SHM_NAME, "r+b")
        mm = mmap.mmap(f.fileno(), ctypes.sizeof(ShmIntegrated))
        shm_data = ShmIntegrated.from_buffer(mm)
    except Exception as e:
        print(f"[Error] Failed to connect to SHM: {e}")
        return

    print(f"[Replayer] Preparing to play '{args.filename}'...")

    # Load CSV Data into Memory (Read entire file)
    records = []
    try:
        with open(args.filename, 'r') as csvfile:
            reader = csv.reader(csvfile)
            header = next(reader) # Skip the header row
            
            for row in reader:
                if not row: continue
                # Convert all data to float and store in list
                records.append([float(x) for x in row])
    except Exception as e:
        print(f"[Error] CSV Parsing Error: {e}")
        return

    if not records:
        print("[Error] No data to replay.")
        return

    print(f"[Replayer] Loaded {len(records)} frames. Starting Replay!")
    print("-------------------------------------------------------------")

    # Start Replay
    start_time_wall = time.time()       # Actual wall-clock start time
    first_record_ts = records[0][0]     # First timestamp in CSV (usually 0.0)

    try:
        for i, row in enumerate(records):
            # Row index mapping based on Logger:
            # [0]Time, [1]Thr, [2]Brk, [3]Str, [4]Ax, [5]Ay, [6]Az, [7]Dist, [8]Traffic
            
            ts_log = row[0]
            
            # Calculate Target Time (Relative Time)
            target_relative_time = ts_log - first_record_ts
            
            # Calculate Current Elapsed Time
            elapsed_wall = time.time() - start_time_wall

            # Wait until the target time is reached
            wait_time = target_relative_time - elapsed_wall
            if wait_time > 0:
                time.sleep(wait_time)

            # --- Write to Shared Memory ---
            info = shm_data.given_info
            
            info.vehicle_command.throttle = row[1]
            info.vehicle_command.brake    = row[2]
            info.vehicle_command.steer_tire_degree = row[3]
            
            info.imu_accel.x_mps2 = row[4]
            info.imu_accel.y_mps2 = row[5]
            info.imu_accel.z_mps2 = row[6]
            
            info.drive_distance.data_km = row[7]
            info.traffic_state.data = int(row[8]) # Traffic state is integer

            # Print Progress (Every 100 frames)
            if i % 100 == 0:
                print(f"\r[Play] Time: {target_relative_time:6.3f}s | Progress: {i}/{len(records)}", end="")

    except KeyboardInterrupt:
        print("\n[Stop] Interrupted by user.")
    
    print("\n[Replayer] Replay finished.")
    mm.close()
    f.close()

if __name__ == "__main__":
    main()