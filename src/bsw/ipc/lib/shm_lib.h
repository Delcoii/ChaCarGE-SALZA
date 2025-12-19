#pragma once
#include "shm_layout.h"

struct ShmIntegrated* init_shared_memory(void);
void detach_shared_memory(struct ShmIntegrated* p_shm);
void destroy_shared_memory(void);