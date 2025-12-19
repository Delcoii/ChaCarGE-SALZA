#pragma once
#include <stdint.h>
#include "struct_imu.h"
#include "struct_traffic_sign.h"
#include "struct_vehicle_command.h"
#include "struct_distance.h"
#include "struct_score.h"

#define SHM_NAME "/shm_integrated"
#define SHM_SIZE (sizeof(IMUAccel) + sizeof(TrafficSignState) + sizeof(VehicleCommand) + sizeof(DrivingScore) + sizeof(DriveDistance))

// shared memory for Data from other apu
typedef struct {
    struct IMUAccel imu_accel;
    struct TrafficSignState traffic_state;
    struct VehicleCommand vehicle_command;
    struct DriveDistance drive_distance;
} ShmData;

// shared memory for Data for this apu
typedef struct {
    struct DrivingScore driving_score;
} ShmInfo;

// Integrated shared memory structure
typedef struct {
    struct ShmData data;
    struct ShmInfo info;
} ShmIntegrated;