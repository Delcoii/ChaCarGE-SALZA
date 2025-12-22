#ifndef __STRUCT_SHARED_MEMORY_H__
#define __STRUCT_SHARED_MEMORY_H__

#include "remote_signal_processing.h"

// --- Event Group Flags ---
#define EVT_REMOTE_UPDATED (1 << 0) // 0x01
#define EVT_STEER_ADC_UPDATED (1 << 1) // 0x02
#define EVT_ALL_UPDATED    (EVT_REMOTE_UPDATED | \
                            EVT_STEER_ADC_UPDATED)


typedef struct {
    // Remote Controller Data
    RemoteSignals_t remote;

    // Steer ADC Data
    uint16_t steer_adc;

} SharedMemory_t;

#endif // __STRUCT_SHARED_MEMORY_H__