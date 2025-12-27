#ifndef __STRUCT_VEHICLE_COMMAND_H__
#define __STRUCT_VEHICLE_COMMAND_H__

#include <stdbool.h>


typedef struct {
    double throttle;            // 0.0 ~ 100.0
    double brake;               // 0.0 ~ 100.0
    double steer_tire_degree;  // -1.0 ~ 1.0 * (MAX_STEER_TIRE_DEG)
    uint32_t steer_adc;        // related to ARR value
    uint8_t mode;              // 0: manual, 1: auto, 2: brake
    bool toggle;
} VehicleCommand_t;



#endif // __STRUCT_VEHICLE_COMMAND_H__