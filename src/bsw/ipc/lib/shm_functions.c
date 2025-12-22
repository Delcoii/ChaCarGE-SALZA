#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "shm_functions.h"

// Initialize shared memory and return pointer to ShmIntegrated structure
ShmIntegrated* init_shared_memory(void)
{
    // 1. create shared memory object
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {   // error handling
        perror("[init_shm] shm_open failed");
        return NULL;
    }

    // 2. setting size of shared memory
    if (ftruncate(shm_fd, SHM_SIZE) == -1)
    {   // error handling
        perror("[init_shm] ftruncate failed");
        close(shm_fd);
        return NULL;
    }
    

    // 3. mapping shared memory
    ShmIntegrated* p_shm = (ShmIntegrated*)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (p_shm == MAP_FAILED)
    {   // error handling
        perror("[init_shm] mmap failed");
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);
    return p_shm;
}

void detach_shared_memory(ShmIntegrated* p_shm)
{
    // 1. invalid test
    if (p_shm == NULL || p_shm == (void*)MAP_FAILED)
    {
        printf("[detach_shm] Warning: Try to close invalid pointer.\n");
        return; 
    }

    // 2. unmapping shared memory
    if (munmap(p_shm, SHM_SIZE) == -1)
    {   // error handling
        perror("[detach_shm] munmap failed");
    }
    else
    {
        printf("[detach_shm] Shared Memory Detached successfully.\n");
    }
}

void destroy_shared_memory(void)
{
    // unlink shared memory
    if (shm_unlink(SHM_NAME) == -1)
    {   // error handling
        perror("[destroy_shm] shm_unlink failed");
    }
    else
    {
        printf("[destroy_shm] Shared Memory Destroyed successfully.\n");
    }
}