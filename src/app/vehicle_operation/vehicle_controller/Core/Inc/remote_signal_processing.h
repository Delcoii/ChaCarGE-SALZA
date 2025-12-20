#ifndef __REMOTE_SIGNAL_PROCESSING_H__
#define __REMOTE_SIGNAL_PROCESSING_H__


#include "main.h"
#include "cmsis_os.h"

// bit flag for checking all channel received
#define RX_FLAG_CH1  (1 << 0) // 0x01
#define RX_FLAG_CH2  (1 << 1) // 0x02
#define RX_FLAG_CH3  (1 << 2) // 0x04
#define RX_FLAG_CH4  (1 << 3) // 0x08
#define RX_FLAG_ALL  (0x0F)   // all channel received


typedef struct {
    uint32_t steering_pulse_width_us;
    uint32_t throttle_pulse_width_us;
    uint32_t mode_pulse_width_us;
    uint32_t brake_pulse_width_us;
} RemoteSignals_t;

// --- Shared Memory Wrapper Structure ---
typedef enum {
    MSG_TYPE_REMOTE_SIGNAL,
    // Add other types here in the future
    // MSG_TYPE_MOTOR_STATUS,
} MsgType_e;

typedef struct {
    MsgType_e type;
    union {
        RemoteSignals_t remote;
        // MotorStatus_t   motor;
    } payload;
} AppMessage_t;


extern osSemaphoreId remote_sig_sem_handle_;

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
RemoteSignals_t GetRemoteSignals(void);



#endif // __REMOTE_SIGNAL_PROCESSING_H__