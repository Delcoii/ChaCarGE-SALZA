#ifndef __STRUCT_REMOTE_SIGNAL_H__
#define __STRUCT_REMOTE_SIGNAL_H__

#include <stdbool.h>

typedef struct {
    uint32_t steering_pulse_width_us;
    uint32_t throttle_pulse_width_us;
    uint32_t toggle_pulse_width_us;
    uint32_t mode_pulse_width_us;
    bool alive;
} RemoteSignals_t;


#endif // __STRUCT_REMOTE_SIGNAL_H__