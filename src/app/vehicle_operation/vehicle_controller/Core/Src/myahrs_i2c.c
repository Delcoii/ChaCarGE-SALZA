#include "myahrs_i2c.h"

osSemaphoreId imu_data_sem_handle_;
static uint8_t i2c_buf[6];

extern I2C_HandleTypeDef hi2c1;  // from main.c

// raw data
int16_t roll_raw, pitch_raw, yaw_raw;
int16_t acc_x_raw, acc_y_raw, acc_z_raw;
int16_t gyro_x_raw, gyro_y_raw, gyro_z_raw;

static IMUData_t imu_data_;
static volatile I2C_State i2c_state = I2C_READ_RPY;


void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance != I2C1) return;

    if (i2c_state == I2C_READ_RPY) {
        // RPY data storage
        roll_raw  = (int16_t)((i2c_buf[1] << 8) | i2c_buf[0]);
        pitch_raw = (int16_t)((i2c_buf[3] << 8) | i2c_buf[2]);
        yaw_raw   = (int16_t)((i2c_buf[5] << 8) | i2c_buf[4]);

        // Read Accel
        i2c_state = I2C_READ_ACC;
        HAL_I2C_Mem_Read_IT(&hi2c1, MYAHRS_I2C_ADDR, REG_C_ACC_X_LOW, I2C_MEMADD_SIZE_8BIT, i2c_buf, 6);
    }
    else if (i2c_state == I2C_READ_ACC) {
        // Accel data storage
        acc_x_raw = (int16_t)((i2c_buf[1] << 8) | i2c_buf[0]);
        acc_y_raw = (int16_t)((i2c_buf[3] << 8) | i2c_buf[2]);
        acc_z_raw = (int16_t)((i2c_buf[5] << 8) | i2c_buf[4]);

        // Read Gyro
        i2c_state = I2C_READ_GYRO;
        HAL_I2C_Mem_Read_IT(&hi2c1, MYAHRS_I2C_ADDR, REG_C_GYRO_X_LOW, I2C_MEMADD_SIZE_8BIT, i2c_buf, 6);
    }
    else if (i2c_state == I2C_READ_GYRO) {
        // Gyro data storage
        gyro_x_raw = (int16_t)((i2c_buf[1] << 8) | i2c_buf[0]);
        gyro_y_raw = (int16_t)((i2c_buf[3] << 8) | i2c_buf[2]);
        gyro_z_raw = (int16_t)((i2c_buf[5] << 8) | i2c_buf[4]);

        // All reading completed -> conversion and output
        i2c_state = I2C_READ_RPY; // Back to read RPY state
        HAL_I2C_Mem_Read_IT(&hi2c1, MYAHRS_I2C_ADDR, REG_ROLL_LOW, I2C_MEMADD_SIZE_8BIT, i2c_buf, 6);
        osSemaphoreRelease(imu_data_sem_handle_);
    }
}

void InitIMUData(void) {
    i2c_state = I2C_READ_RPY;
    HAL_I2C_Mem_Read_IT(&hi2c1, MYAHRS_I2C_ADDR, REG_ROLL_LOW, I2C_MEMADD_SIZE_8BIT, i2c_buf, 6);
}



IMUData_t GetIMUData(void) {
    if (osSemaphoreWait(imu_data_sem_handle_, 100) == osOK) {
        imu_data_.roll_deg  = roll_raw  * RPY_SCALE;
        imu_data_.pitch_deg = pitch_raw * RPY_SCALE;
        imu_data_.yaw_deg   = yaw_raw   * RPY_SCALE;
        imu_data_.acc_x_mps2 = acc_x_raw * ACC_SCALE;
        imu_data_.acc_y_mps2 = acc_y_raw * ACC_SCALE * (-1.0);
        imu_data_.acc_z_mps2 = acc_z_raw * ACC_SCALE;
        imu_data_.gyro_x_dps = gyro_x_raw * GYRO_SCALE;
        imu_data_.gyro_y_dps = gyro_y_raw * GYRO_SCALE;
        imu_data_.gyro_z_dps = gyro_z_raw * GYRO_SCALE;

        return imu_data_;
    }


    // retry
    i2c_state = I2C_READ_RPY;
    if(HAL_I2C_Mem_Read_IT(&hi2c1, MYAHRS_I2C_ADDR, REG_ROLL_LOW, I2C_MEMADD_SIZE_8BIT, i2c_buf, 6) != HAL_OK) {
         HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET); 
    }

    // fatal error
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    return (IMUData_t){-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
}