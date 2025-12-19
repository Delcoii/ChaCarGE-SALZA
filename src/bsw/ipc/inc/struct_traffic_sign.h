#pragma once
#include <stdint.h>

#define MQ_NAME "/traffic_sign_state"

typedef struct {
    uint32_t traffic_state;
} TrafficSignState;