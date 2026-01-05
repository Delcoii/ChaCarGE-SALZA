#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h> // Required for clock_nanosleep, clock_gettime
#include "shm_functions.h"
#include "driving_score_calc.h"

// Define Loop Period: 10ms = 10,000,000 nanoseconds (100Hz)
#define LOOP_PERIOD_NS 10000000LL

// Global variables for signal handling
ShmIntegrated* g_p_shm = NULL;
uint8_t keep_running = 1;

// Signal Handler for Graceful Shutdown (Ctrl+C)
void signal_handler(int32_t sig) {
    if (sig == SIGINT) {
        printf("\n[Main] Shutdown signal received. Stopping loop...\n");
        keep_running = 0;
    }
}

// Helper function to add nanoseconds to timespec
void timespec_add_ns(struct timespec *t, uint32_t ns) {
    t->tv_nsec += ns;
    while (t->tv_nsec >= 1000000000) {
        t->tv_nsec -= 1000000000;
        t->tv_sec += 1;
    }
}

int main() {
    // 1. Register Signal Handler
    signal(SIGINT, signal_handler);
    
    printf("[Main] Safety Score System Started (High Precision Mode: 10ms/100Hz)\n");
    printf("[Main] Press Ctrl+C to stop safely.\n");
    
    // 2. Initialize Shared Memory
    g_p_shm = init_shared_memory();
    if (g_p_shm == NULL) {
        fprintf(stderr, "[Main] Error: Failed to attach shared memory.\n");
        return 1;
    }

    
    //g_p_shm->given_info.bUseDrivingScoreChecking = 1; // Enable by default
    
    // 3. Initialize Algorithm State
    AlgoState algo_state;
    init_algo_state(&algo_state);
    
    // Initialize event counts in the algorithm state
    algo_state.sudden_accel_count = 0;
    algo_state.sudden_curve_count = 0;
    algo_state.bump_count = 0;
    algo_state.signal_violation_count = 0;
    
    // 4. Variable for state update
    uint8_t prev_control_flag = 0;

    // 4. Setup High Precision Timing
    struct timespec next_activation;
    // Get current time from monotonic clock (system uptime based, unaffected by system time changes)
    clock_gettime(CLOCK_MONOTONIC, &next_activation);

    // 5. Main Real-time Loop
    while (keep_running) {

        // read the control flag
        uint8_t curr_control_flag = g_p_shm->given_info.bUseDrivingScoreChecking;

        /* ------------------------------------------------------- */
        /* Step A: Execute Algorithm                               */
        /* (Read Input -> Process Logic -> Write Output)           */
        /* ------------------------------------------------------- */
        if(curr_control_flag) {
            // If control flag is enabled, run the driving score update
            if(prev_control_flag == 0) {
                algo_state.is_first_loop = 1; // Reset first loop flag
            }
            update_driving_score(&g_p_shm->given_info, &g_p_shm->generated_info, &algo_state);
        }
        else {
            // If control flag is disabled, do not run the algorithm
            if(prev_control_flag == 1) {
                g_p_shm->generated_info.driving_score_type.score_type = SCORE_TYPE_NONE;
                g_p_shm->generated_info.driving_score_type.count = 0;
            }
        }

        // Update current control flag state
        prev_control_flag = curr_control_flag;

        /* ------------------------------------------------------- */
        /* Step B: Calculate Next Wake-up Time                     */
        /* Add exactly 10ms to the target time.                    */
        /* This prevents time drift even if the algorithm is slow. */
        /* ------------------------------------------------------- */
        timespec_add_ns(&next_activation, LOOP_PERIOD_NS);

        /* ------------------------------------------------------- */
        /* Step C: Precision Sleep (Absolute Time)                 */
        /* Sleep until the specific 'next_activation' time.        */
        /* If the task finished early, it sleeps the remainder.    */
        /* ------------------------------------------------------- */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_activation, NULL);
    }

    // 6. Cleanup Resources
    printf("[Main] Cleaning up resources...\n");
    detach_shared_memory(g_p_shm);
    
    printf("[Main] Process Terminated Successfully.\n");
    return 0;
}