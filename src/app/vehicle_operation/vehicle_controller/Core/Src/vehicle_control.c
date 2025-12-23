#include "vehicle_control.h"

extern TIM_HandleTypeDef htim1;     // from main.c


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


VehicleCommand_t PulseToVehicleCommand(RemoteSignals_t remote) {
    VehicleCommand_t command;

    // remote signal error check
    if (remote.mode_pulse_width_us < MIN_PULSE_WIDTH_US - PULSE_DETECT_ERR_OFFSET ||
        remote.mode_pulse_width_us > MAX_PULSE_WIDTH_US + PULSE_DETECT_ERR_OFFSET) {
        command.mode = ERROR_MODE;
        command.throttle = 0.0;
        command.brake = 0.0;
        command.steer_tire_degree = 0.0;
        command.toggle = false;
        return command;
    }

    // defining mode
    if (remote.mode_pulse_width_us >= 900 && remote.mode_pulse_width_us < 1200) {
        command.mode = MANUAL_MODE;
    } else if (remote.mode_pulse_width_us >= 1200 && remote.mode_pulse_width_us < 1700) {
        command.mode = AUTO_MODE;
    } else if (remote.mode_pulse_width_us >= 1700 && remote.mode_pulse_width_us < 2100) {
        command.mode = BRAKE_MODE;
    }

    // defining throttle, brake(reverse), accel
    if (remote.throttle_pulse_width_us > NEUTRAL_PULSE_WIDTH_US) {
        command.throttle = LinearMapping(remote.throttle_pulse_width_us,
                                        NEUTRAL_PULSE_WIDTH_US, MAX_PULSE_WIDTH_US,
                                        0, MAX_THROTTLE);
        command.brake = 0.0;
    } else {
        command.throttle = 0.0;
        command.brake = LinearMapping(remote.throttle_pulse_width_us,
                                      NEUTRAL_PULSE_WIDTH_US, MIN_PULSE_WIDTH_US,
                                      0, MAX_THROTTLE);
    }

    command.steer_tire_degree = 0; // TODO : change by PID control

    return command;
}


void StopMotor(void) {
    // IN1 & IN2 Reset
    HAL_GPIO_WritePin(LEFT_IN1_GPIO_Port, LEFT_IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEFT_IN2_GPIO_Port, LEFT_IN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RIGHT_IN1_GPIO_Port, RIGHT_IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RIGHT_IN2_GPIO_Port, RIGHT_IN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEER_IN1_GPIO_Port, STEER_IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEER_IN2_GPIO_Port, STEER_IN2_Pin, GPIO_PIN_RESET);

    // PWM Duty Reset
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
}


void MoveForward(double pedal, uint32_t arr) {
    HAL_GPIO_WritePin(RIGHT_IN1_GPIO_Port, RIGHT_IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RIGHT_IN2_GPIO_Port, RIGHT_IN2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEFT_IN1_GPIO_Port, LEFT_IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEFT_IN2_GPIO_Port, LEFT_IN2_Pin, GPIO_PIN_RESET);

    uint32_t ccr = (double)(pedal / MAX_THROTTLE) * arr;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr);      // RIGHT MOTOR
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, ccr);      // LEFT MOTOR
}

void MoveBackward(double pedal, uint32_t arr) {
    HAL_GPIO_WritePin(RIGHT_IN1_GPIO_Port, RIGHT_IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RIGHT_IN2_GPIO_Port, RIGHT_IN2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEFT_IN1_GPIO_Port, LEFT_IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEFT_IN2_GPIO_Port, LEFT_IN2_Pin, GPIO_PIN_SET);

    uint32_t ccr = (double)(pedal / MAX_THROTTLE) * arr;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, ccr);      // RIGHT MOTOR
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, ccr);      // LEFT MOTOR
}

// TODO : change by PID control
void MoveSteer(double steer_tire_degree, uint32_t arr) {
    return;
}
