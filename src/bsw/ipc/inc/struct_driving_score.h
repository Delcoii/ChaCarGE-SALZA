#ifndef STRUCT_DRIVING_SCORE_H
#define STRUCT_DRIVING_SCORE_H

#include <stdint.h>

enum {
    SCORE_BUMP = 0,
    SCORE_OVER_SPEED,
    SCORE_SUDDEN_ACCEL,
    SCORE_SUDDEN_CURVE,
    SCORE_IGNORE_SIGN,
    SCORE_V2V_DISTANCE,
}

typedef struct{
    double total_score;
    uint8_t score_type;
} DrivingScore;

#endif // STRUCT_DRIVING_SCORE_H