#include "remote_signal_processing.h"

osSemaphoreId remote_sig_sem_handle_;

static volatile RemoteSignals_t remote_signals = {0};
static volatile uint8_t rx_sync_flags_ = 0; 
static volatile uint32_t ch1_rising_edge_us_ = 0;
static volatile uint32_t ch2_rising_edge_us_ = 0;
static volatile uint32_t ch3_rising_edge_us_ = 0;
static volatile uint32_t ch4_rising_edge_us_ = 0;
static volatile uint32_t current_capture_us_ = 0;


static inline uint32_t CalcPulseWidth(uint32_t rising, uint32_t falling) {
    if (falling >= rising) return falling - rising;
    else return (0xFFFFFFFF - rising) + falling;
}

// BaseType_t xHigherPriorityTaskWoken = pdFALSE;
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        current_capture_us_ = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

        if (HAL_GPIO_ReadPin(TIM2_CH1_GPIO_Port, TIM2_CH1_Pin) == GPIO_PIN_SET) {
            ch1_rising_edge_us_ = current_capture_us_;
        } else {
            remote_signals.steering_pulse_width_us = CalcPulseWidth(ch1_rising_edge_us_, current_capture_us_);
            rx_sync_flags_ |= RX_FLAG_CH1;
        }
    }
    else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
        current_capture_us_ = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        if (HAL_GPIO_ReadPin(TIM2_CH2_GPIO_Port, TIM2_CH2_Pin) == GPIO_PIN_SET) {
            ch2_rising_edge_us_ = current_capture_us_;
        } else {
            remote_signals.throttle_pulse_width_us = CalcPulseWidth(ch2_rising_edge_us_, current_capture_us_);
            rx_sync_flags_ |= RX_FLAG_CH2;
        }
    }
    else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
        current_capture_us_ = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
        if (HAL_GPIO_ReadPin(TIM2_CH3_GPIO_Port, TIM2_CH3_Pin) == GPIO_PIN_SET) {
            ch3_rising_edge_us_ = current_capture_us_;
        } else {
            remote_signals.mode_pulse_width_us = CalcPulseWidth(ch3_rising_edge_us_, current_capture_us_);
            rx_sync_flags_ |= RX_FLAG_CH3;
        }
    }
    else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
        current_capture_us_ = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);
        if (HAL_GPIO_ReadPin(TIM2_CH4_GPIO_Port, TIM2_CH4_Pin) == GPIO_PIN_SET) {
            ch4_rising_edge_us_ = current_capture_us_;
        } else {
            remote_signals.brake_pulse_width_us = CalcPulseWidth(ch4_rising_edge_us_, current_capture_us_);
            rx_sync_flags_ |= RX_FLAG_CH4;
        }
    }

    // if every channel is received, release the semaphore
    if (rx_sync_flags_ == RX_FLAG_ALL) {
        osSemaphoreRelease(remote_sig_sem_handle_);
        
        rx_sync_flags_ = 0;
        HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
    }
}


RemoteSignals_t GetRemoteSignals(void) {
    if (osSemaphoreWait(remote_sig_sem_handle_, osWaitForever) == osOK) {
        return remote_signals;
    }

    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    return remote_signals; // Should not reach here
}