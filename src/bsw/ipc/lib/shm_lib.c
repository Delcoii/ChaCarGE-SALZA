#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "shm_lib.h"

// Initialize shared memory and return pointer to ShmIntegrated structure
struct ShmIntegrated* init_shared_memory(void)
{
    // 1. create shared memory object
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {   // error handling
        perror("[SHM_LIB] shm_open failed");
        return NULL;
    }

    // 2. setting size of shared memory
    if (ftruncate(shm_fd, SHM_SIZE) == -1)
    {   // error handling
        perror("[SHM_LIB] ftruncate failed");
        close(shm_fd);
        return NULL;
    }
    

    // 3. mapping shared memory
    struct ShmIntegrated* p_shm = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (p_shm == MAP_FAILED)
    {   // error handling
        perror("[SHM_LIB] mmap failed");
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);
    return p_shm;
}

void detach_shared_memory(struct ShmIntegrated* p_shm)
{
    // 1. invalid test
    if (p_shm == NULL || p_shm == (void*)MAP_FAILED)
    {
        printf("[SHM_LIB] Warning: Try to close invalid pointer.\n");
        return; 
    }

    // 2. unmapping shared memory
    if (munmap(p_shm, SHM_SIZE) == -1)
    {   // error handling
        perror("[SHM_LIB] munmap failed");
    }
    else
    {
        printf("[SHM_LIB] Shared Memory Detached successfully.\n");
    }
}

void destroy_shared_memory(void)
{
    // unlink shared memory
    if (shm_unlink(SHM_NAME) == -1)
    {   // error handling
        perror("[SHM_LIB] shm_unlink failed");
    }
    else
    {
        printf("[SHM_LIB] Shared Memory Destroyed successfully.\n");
    }
}