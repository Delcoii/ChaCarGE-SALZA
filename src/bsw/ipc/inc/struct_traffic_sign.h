#ifndef STRUCT_TRAFFIC_SIGN_H
#define STRUCT_TRAFFIC_SIGN_H

#include <stdint.h>

#define MQ_TRAFFIC_SIGN_NAME "/traffic_sign_state"

#define TRAFFIC_STATE_RED 0
#define TRAFFIC_STATE_YELLOW 1
#define TRAFFIC_STATE_GREEN 2

typedef struct {
    uint32_t data;
} TrafficSignState;

#endif // STRUCT_TRAFFIC_SIGN_H