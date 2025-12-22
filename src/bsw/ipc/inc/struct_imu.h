#ifndef STRUCT_IMU_H
#define STRUCT_IMU_H

#include <stdint.h>

#define MQ_IMU_NAME "/imu_accel"

typedef struct {
    double x_mps2;
    double y_mps2;
    double z_mps2;
} IMUAccel;

#endif // STRUCT_IMU_H