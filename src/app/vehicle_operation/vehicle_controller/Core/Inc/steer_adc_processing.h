#ifndef __STEER_ADC_PROCESSING_H__
#define __STEER_ADC_PROCESSING_H__

#include "main.h"
#include "cmsis_os.h"

extern osSemaphoreId steer_adc_sem_handle_;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
uint32_t GetSteerADCValue(void);

#endif // __STEER_ADC_PROCESSING_H__