#ifndef SHM_FUNCTIONS_H
#define SHM_FUNCTIONS_H
#include "shm_layout.h"

struct ShmIntegrated* init_shared_memory(void);
void detach_shared_memory(struct ShmIntegrated* p_shm);
void destroy_shared_memory(void);

#endif // SHM_FUNCTIONS_H