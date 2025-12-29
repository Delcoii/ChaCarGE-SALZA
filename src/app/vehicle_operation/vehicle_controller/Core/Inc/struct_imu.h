#ifndef __STRUCT_IMU_H__
#define __STRUCT_IMU_H__

typedef struct {
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
    float acc_x_mps2;
    float acc_y_mps2;
    float acc_z_mps2;
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
} IMUData_t;


#endif // __STRUCT_IMU_H__