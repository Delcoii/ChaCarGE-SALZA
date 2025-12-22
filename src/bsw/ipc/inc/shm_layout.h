#ifndef SHM_LAYOUT_H
#define SHM_LAYOUT_H

#include <stdint.h>
#include "struct_imu.h"
#include "struct_traffic_sign.h"
#include "struct_vehicle_command.h"
#include "struct_distance.h"
#include "struct_score.h"

// shared memory for Data from other apu
typedef struct {
    struct IMUAccel imu_accel;
    struct TrafficSignState traffic_state;
    struct VehicleCommand vehicle_command;
    struct DriveDistance drive_distance;
} ShmGivenInfo;

// shared memory for Data for main apu
typedef struct {
    struct DrivingScore driving_score;
} ShmGeneratedInfo;

// Integrated shared memory structure
typedef struct {
    struct ShmGivenInfo given_info;
    struct ShmGeneratedInfo generated_info;
} ShmIntegrated;

#define SHM_NAME "/shm_integrated"
#define SHM_SIZE (sizeof(ShmIntegrated))

#endif // SHM_LAYOUT_H