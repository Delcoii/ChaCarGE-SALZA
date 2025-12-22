#ifndef STRUCT_VEHICLE_COMMAND_H
#define STRUCT_VEHICLE_COMMAND_H

#include <stdint.h>

#define MQ_VEHICLE_COMMAND_NAME "/vehicle_command"

typedef struct {
    double throttle;
    double brake;
    double steer_tire_degree;
} VehicleCommand;

#endif // STRUCT_VEHICLE_COMMAND_H