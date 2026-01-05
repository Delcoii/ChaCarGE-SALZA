#ifndef STRUCT_DRIVING_SCORE_TYPE_H
#define STRUCT_DRIVING_SCORE_TYPE_H

#include <stdint.h>

enum {
    SCORE_BUMP = 0,
    SCORE_SUDDEN_ACCEL,
    SCORE_SUDDEN_CURVE,
    SCORE_IGNORE_SIGN,
    
    /* not using this time */
    SCORE_V2V_DISTANCE,
    SCORE_OVER_SPEED,
    
    // no issue
    SCORE_TYPE_NONE
};

typedef struct{
    uint16_t count;
    uint16_t score_type;
} DrivingScoreType;

#endif // STRUCT_DRIVING_SCORE_TYPE_H