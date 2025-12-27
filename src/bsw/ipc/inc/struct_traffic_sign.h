#ifndef STRUCT_TRAFFIC_SIGN_H
#define STRUCT_TRAFFIC_SIGN_H

#include <stdint.h>

#define MQ_TRAFFIC_SIGN_NAME "/traffic_sign_state"

enum {
    TRAFFIC_STATE_NONE = 0,
    TRAFFIC_STATE_RED,
    TRAFFIC_STATE_YELLOW,
    TRAFFIC_STATE_GREEN,
};

typedef struct {
    uint8_t sign_state;
} TrafficSignState;

#endif // STRUCT_TRAFFIC_SIGN_H