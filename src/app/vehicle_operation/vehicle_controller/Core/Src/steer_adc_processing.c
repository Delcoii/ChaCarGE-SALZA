#include "steer_adc_processing.h"

osSemaphoreId steer_adc_sem_handle_;
uint32_t adc_val;

extern ADC_HandleTypeDef hadc1;  // from main.c

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    osSemaphoreRelease(steer_adc_sem_handle_);
}

uint32_t GetSteerADCValue(void) {
    HAL_ADC_Start_IT(&hadc1);

    if (osSemaphoreWait(steer_adc_sem_handle_, 100) == osOK) {
        return HAL_ADC_GetValue(&hadc1);
    }

    // fatal error
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    return -1;
}