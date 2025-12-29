#ifndef __STRUCT_SHARED_MEMORY_H__
#define __STRUCT_SHARED_MEMORY_H__


#include "struct_remote_signal.h"
#include "struct_vehicle_command.h"

// Event Group Flags
#define EVT_REMOTE_UPDATED (1 << 0)             // 0x01
#define EVT_STEER_ADC_UPDATED (1 << 1)          // 0x02
#define EVT_VEHICLE_COMMAND_UPDATED (1 << 2)    // 0x04
// IMU Event update data
#define EVT_ALL_UPDATED    (EVT_REMOTE_UPDATED | \
                            EVT_STEER_ADC_UPDATED | \
                            EVT_VEHICLE_COMMAND_UPDATED)


typedef struct {
    // Remote Controller Data
    RemoteSignals_t remote;

    // Steer ADC Data
    uint16_t steer_adc;

    // Vehicle Command Data
    VehicleCommand_t vehicle_command;
    
} SharedMemory_t;

#endif // __STRUCT_SHARED_MEMORY_H__
