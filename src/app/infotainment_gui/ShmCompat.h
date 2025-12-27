#ifndef SHM_COMPAT_H
#define SHM_COMPAT_H

#include <stdint.h>

// Override broken BSW header guard and supply the needed types locally.
#ifndef STRUCT_DRIVING_SCORE_H
#define STRUCT_DRIVING_SCORE_H
enum {
    SCORE_BUMP = 0,
    SCORE_OVER_SPEED,
    SCORE_SUDDEN_ACCEL,
    SCORE_SUDDEN_CURVE,
    SCORE_IGNORE_SIGN,
    SCORE_V2V_DISTANCE,
};

typedef struct{
    double total_score;
    uint8_t score_type;
} DrivingScore;
#endif

#include "../../bsw/ipc/inc/shm_layout.h"

#ifdef __cplusplus
extern "C" {
#endif
ShmIntegrated* init_shared_memory(void);
void detach_shared_memory(ShmIntegrated* p_shm);
void destroy_shared_memory(void);
#ifdef __cplusplus
}
#endif

#endif // SHM_COMPAT_H
