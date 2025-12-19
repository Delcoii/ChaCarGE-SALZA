#pragma once
#include <stdint.h>

#define MQ_NAME "/imu_accel"

typedef struct {
    double x_mps2;
    double y_mps2;
    double z_mps2;
} IMUAccel;
