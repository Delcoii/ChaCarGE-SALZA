#ifndef __STRUCT_SHARED_MEMORY_H__
#define __STRUCT_SHARED_MEMORY_H__

#include "remote_signal_processing.h"

// --- Event Group Flags ---
#define EVT_REMOTE_UPDATED (1 << 0) // 0x01
#define EVT_MOTOR_UPDATED  (1 << 1) // 0x02
#define EVT_ALL_UPDATED    (EVT_REMOTE_UPDATED | EVT_MOTOR_UPDATED)


typedef struct {
    // 1. Remote Controller Data
    RemoteSignals_t remote;
    uint32_t last_update_remote;

    // 2. Motor Status Data (Example)
    // MotorStatus_t motor;
    // uint32_t last_update_motor;

} SharedMemory_t;


#endif // __STRUCT_SHARED_MEMORY_H__