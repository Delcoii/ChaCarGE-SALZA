#pragma once
#include <stdint.h>

#define SHM_NAME "/shm_score"

typedef struct{
    double total_score;
    int32_t score_type;
} DrivingScore;