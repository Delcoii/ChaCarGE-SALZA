#include "steer_adc_processing.h"

osSemaphoreId steer_adc_sem_handle_;
extern ADC_HandleTypeDef hadc1;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        osSemaphoreRelease(steer_adc_sem_handle_);
    }
}

uint16_t GetSteerADCValue(void) {
    if (osSemaphoreWait(steer_adc_sem_handle_, osWaitForever) == osOK) {
        return HAL_ADC_GetValue(&hadc1);
    }

    // fatal error
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    return -1;
}