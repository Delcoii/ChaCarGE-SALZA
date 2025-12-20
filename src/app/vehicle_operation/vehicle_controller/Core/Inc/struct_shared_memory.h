#ifndef __APP_MESSAGE_H__
#define __APP_MESSAGE_H__

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os.h"

// Message Type Definition
typedef enum {
    MSG_TYPE_REMOTE_SIGNAL,
    MSG_TYPE_MOTOR_STATUS,
    // Add other types here
} MsgType_e;


// Remote Control Data
typedef struct {
    uint32_t steering_pulse_width_us;
    uint32_t throttle_pulse_width_us;
    uint32_t mode_pulse_width_us;
    uint32_t brake_pulse_width_us;
    bool alive;
} RemoteSignals_t;

// Motor Status Data (Example)
typedef struct {
    float current_speed;
    float current_steering_angle;
    uint8_t error_code;
} MotorStatus_t;



// Shared Memory Wrapper Structure
typedef struct {
    MsgType_e type;
    union {
        RemoteSignals_t remote;
        MotorStatus_t   motor;
        // Add other payload structures here
    } payload;
} SharedMemory_t;



#endif // __APP_MESSAGE_H__
