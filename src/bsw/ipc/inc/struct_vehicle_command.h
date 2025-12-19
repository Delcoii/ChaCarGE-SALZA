#pragma once
#include <stdint.h>

#define MQ_NAME "/vehicle_command"

typedef struct {
    double throttle;
    double brake;
    double steer_tire_degree;
} VehicleCommand;
