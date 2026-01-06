#include "ShmCompat.h"

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

extern "C" ShmIntegrated* init_shared_memory(void)
{
    bool created = true;
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd == -1 && errno == EEXIST) {
        // Already exists; open without recreating/clearing
        created = false;
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    }
    if (shm_fd == -1)
    {
        perror("[init_shm] shm_open failed");
        return NULL;
    }

    if (created) {
        if (ftruncate(shm_fd, SHM_SIZE) == -1)
        {
            perror("[init_shm] ftruncate failed");
            close(shm_fd);
            return NULL;
        }
    }

    ShmIntegrated* p_shm = (ShmIntegrated*)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (p_shm == MAP_FAILED)
    {
        perror("[init_shm] mmap failed");
        close(shm_fd);
        return NULL;
    }
    if (created) {
        // Ensure clean initialization only when creating the segment
        memset(p_shm, 0, SHM_SIZE);
        p_shm->generated_info.driving_score_type.score_type = SCORE_TYPE_NONE;
    }

    close(shm_fd);
    return p_shm;
}

extern "C" void detach_shared_memory(ShmIntegrated* p_shm)
{
    if (p_shm == NULL || p_shm == (void*)MAP_FAILED)
    {
        printf("[detach_shm] Warning: Try to close invalid pointer.\n");
        return;
    }

    if (munmap(p_shm, SHM_SIZE) == -1)
    {
        perror("[detach_shm] munmap failed");
    }
    else
    {
        printf("[detach_shm] Shared Memory Detached successfully.\n");
    }
}

extern "C" void destroy_shared_memory(void)
{
    if (shm_unlink(SHM_NAME) == -1)
    {
        perror("[destroy_shm] shm_unlink failed");
    }
    else
    {
        printf("[destroy_shm] Shared Memory Destroyed successfully.\n");
    }
}
