#include "vehicle_control.h"

double PIDControl(double ref, double sense) {
    static double prev_error = 0.0;
    static uint32_t prev_time_us = 0;

    uint32_t now_time_us = HAL_GetTick();
    uint32_t dt_us = now_time_us - prev_time_us;
    prev_time_us = now_time_us;
    

    double error = (ref - sense);
    double P = error * KP;
    double I = error * dt_us * KI;
    double D = (error - prev_error) / dt_us * KD;


    prev_error = error;
    return P + I + D;
}


double LinearMapping(double x, double in_min, double in_max, double out_min, double out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void StopMotor(void) {
    HAL_GPIO_WritePin(LEFT_IN1_GPIO_Port, LEFT_IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEFT_IN2_GPIO_Port, LEFT_IN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RIGHT_IN1_GPIO_Port, RIGHT_IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RIGHT_IN2_GPIO_Port, RIGHT_IN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEER_IN1_GPIO_Port, STEER_IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEER_IN2_GPIO_Port, STEER_IN2_Pin, GPIO_PIN_RESET);

}

