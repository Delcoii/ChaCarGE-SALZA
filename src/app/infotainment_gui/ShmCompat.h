#ifndef SHM_COMPAT_H
#define SHM_COMPAT_H

#include <stdint.h>
#include "../../bsw/ipc/inc/shm_layout.h"

// Scoped ScoreType to use in C++ code (mirrors struct_driving_score_type.h)
enum class ScoreType : uint16_t {
    SCORE_BUMP = 0,
    SCORE_SUDDEN_ACCEL,
    SCORE_SUDDEN_CURVE,
    SCORE_IGNORE_SIGN,
    SCORE_V2V_DISTANCE,
    SCORE_OVER_SPEED,
    SCORE_TYPE_NONE
};

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
