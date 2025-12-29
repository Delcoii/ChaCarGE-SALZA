#ifndef __STRUCT_SHARED_MEMORY_H__
#define __STRUCT_SHARED_MEMORY_H__


#include "struct_remote_signal.h"
#include "struct_vehicle_command.h"

// Event Group Flags
#define EVT_REMOTE_UPDATED_FOR_LOG      (1 << 0)        // 0x01
#define EVT_REMOTE_UPDATED_FOR_CAN      (1 << 1)        // 0x02
#define EVT_STEER_ADC_UPDATED_FOR_LOG   (1 << 2)        // 0x04
#define EVT_STEER_ADC_UPDATED_FOR_CAN   (1 << 3)        // 0x08
#define EVT_VEHICLE_COMMAND_UPDATED_FOR_LOG (1 << 4)    // 0x08
#define EVT_VEHICLE_COMMAND_UPDATED_FOR_CAN (1 << 5)    // 0x10


// Event Group Flags for Log
#define EVT_ALL_UPDATED_FOR_LOG    (EVT_REMOTE_UPDATED_FOR_LOG | \
                                    EVT_STEER_ADC_UPDATED_FOR_LOG | \
                                    EVT_VEHICLE_COMMAND_UPDATED_FOR_LOG)

// Event Group Flags for CAN
#define EVT_ALL_UPDATED_FOR_CAN    (EVT_REMOTE_UPDATED_FOR_CAN | \
                                    EVT_STEER_ADC_UPDATED_FOR_CAN | \
                                    EVT_VEHICLE_COMMAND_UPDATED_FOR_CAN)


#define EVT_ALL_UPDATED    (EVT_ALL_UPDATED_FOR_LOG | \
                            EVT_ALL_UPDATED_FOR_CAN)



typedef struct {
    // Remote Controller Data
    RemoteSignals_t remote;

    // Steer ADC Data
    uint16_t steer_adc;

    // Vehicle Command Data
    VehicleCommand_t vehicle_command;
    
} SharedMemory_t;

#endif // __STRUCT_SHARED_MEMORY_H__
