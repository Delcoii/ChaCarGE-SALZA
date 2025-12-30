#ifndef __MYAHRS_I2C_H__
#define __MYAHRS_I2C_H__

#include "main.h"
#include "cmsis_os.h"

#include "struct_imu.h"

#define MYAHRS_I2C_ADDR     (0x20 << 1)
#define REG_WHO_AM_I        0x01

#define REG_C_ACC_X_LOW     0x22  // Calibrated Accelerometer X Low
#define REG_C_GYRO_X_LOW    0x28  // Calibrated Gyroscope X Low
#define REG_ROLL_LOW        0x36  // Roll Low Register Addr

// Scale Factors
#define GRAVITY_MSS         9.80665f
#define ACC_SCALE           ((16.0f * GRAVITY_MSS) / 32768.0f) 
#define GYRO_SCALE          (2000.0f / 32768.0f)
#define RPY_SCALE           (180.0f / 32768.0f)



typedef enum {
    I2C_READ_RPY,
    I2C_READ_ACC,
    I2C_READ_GYRO
} I2C_State;


extern osSemaphoreId imu_data_sem_handle_;


void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c);
void InitIMUData(void);
IMUData_t GetIMUData(void);

#endif // __MYAHRS_I2C_H__