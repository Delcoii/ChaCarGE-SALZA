#ifndef SHM_LAYOUT_H
#define SHM_LAYOUT_H

#include <stdint.h>
#include "struct_imu.h"
#include "struct_traffic_sign.h"
#include "struct_vehicle_command.h"
#include "struct_distance.h"
#include "struct_driving_score_type.h"

// shared memory for Data from other apu
typedef struct {
    IMUAccel imu_accel;
    TrafficSignState traffic_state;
    VehicleCommand vehicle_command;
    DriveDistance drive_distance;
    uint8_t bUseDrivingScoreChecking;
} ShmGivenInfo;

// shared memory for Data for main apu
typedef struct {
    DrivingScoreType driving_score_type;
    double total_score;
} ShmGeneratedInfo;

// Integrated shared memory structure
typedef struct {
    ShmGivenInfo given_info;
    ShmGeneratedInfo generated_info;
} ShmIntegrated;

#define SHM_NAME "/shm_integrated"
#define SHM_SIZE (sizeof(ShmIntegrated))

#endif // SHM_LAYOUT_H
