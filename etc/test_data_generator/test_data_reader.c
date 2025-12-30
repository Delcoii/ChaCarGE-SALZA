#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h> // Required for clock_nanosleep
#include "shm_functions.h"

// For Enum String Conversion
// #include "struct_driving_score.h"
// #include "struct_traffic_sign.h"

// Define Refresh Rate: 100ms (10Hz) for human readability
// 100,000,000 nanoseconds
#define UI_REFRESH_PERIOD_NS 100000000LL

// Global pointer for cleanup
ShmIntegrated* g_p_shm = NULL;
int keep_running = 1;

// Signal Handler (Ctrl+C)
void signal_handler(int sig) {
    if (sig == SIGINT) {
        keep_running = 0;
    }
}

// Helper: Add nanoseconds to timespec
void timespec_add_ns(struct timespec *t, long ns) {
    t->tv_nsec += ns;
    while (t->tv_nsec >= 1000000000) {
        t->tv_nsec -= 1000000000;
        t->tv_sec += 1;
    }
}

// Helper: Convert Traffic Enum to String
const char* get_traffic_str(int status) {
    switch (status) {
        case TRAFFIC_STATE_RED:    return "RED 🔴";
        case TRAFFIC_STATE_YELLOW: return "YELLOW 🟡";
        case TRAFFIC_STATE_GREEN:  return "GREEN 🟢";
        default:                   return "NONE ⚪";
    }
}

// Helper: Convert Score Type Enum to String
const char* get_score_type_str(int type) {
    switch (type) {
        case SCORE_BUMP:          return "SPEED BUMP";
        case SCORE_SUDDEN_ACCEL:  return "SUDDEN ACCEL/BRAKE 🚀";
        case SCORE_SUDDEN_CURVE:  return "SUDDEN CURVE ⤵️";
        case SCORE_IGNORE_SIGN:   return "SIGNAL VIOLATION 🚨";
        case SCORE_TYPE_NONE:     return "Safe Driving ✅";
        default:                  return "Unknown";
    }
}

int main() {
    signal(SIGINT, signal_handler);

    printf("[Reader] Connecting to Shared Memory...\n");
    g_p_shm = init_shared_memory();
    if (!g_p_shm) {
        fprintf(stderr, "[Reader] Error: Failed to attach SHM.\n");
        return 1;
    }

    printf("[Reader] Monitoring Started. Press Ctrl+C to exit.\n");

    // Setup High Precision Timing for UI
    struct timespec next_refresh;
    clock_gettime(CLOCK_MONOTONIC, &next_refresh);

    // Loop for Monitoring
    while (keep_running) {
        // --- 1. Draw UI ---
        
        // Clear Screen (Home cursor + Clear) - Reduces flickering
        printf("\033[H\033[J");

        printf("===================================================\n");
        printf("         VEHICLE SAFETY MONITORING SYSTEM          \n");
        printf("===================================================\n\n");

        /* Input Sensors */
        printf(" [ INPUT SENSORS ]\n");
        printf(" ---------------------------------------------------\n");
        printf("  Throttle       : %5.1f %% \n", g_p_shm->given_info.vehicle_command.throttle);
        printf("  Brake          : %5.1f %% \n", g_p_shm->given_info.vehicle_command.brake);
        printf("  Steering Angle : %5.1f deg \n", g_p_shm->given_info.vehicle_command.steer_tire_degree);
        printf("  Traffic Light  : %s \n", get_traffic_str(g_p_shm->given_info.traffic_state.sign_state));
        printf("  IMU Accel (X/Y): %5.2f / %5.2f m/s^2 \n", 
               g_p_shm->given_info.imu_accel.x_mps2, 
               g_p_shm->given_info.imu_accel.y_mps2);
        printf("  Total Distance : %8.4f km \n", g_p_shm->given_info.drive_distance.data_km);
        printf("\n");

        /* Algorithm Outputs */
        printf(" [ ALGORITHM ANALYSIS ]\n");
        printf(" ---------------------------------------------------\n");
        
        // Event Detection Display
        int current_type = g_p_shm->generated_info.driving_score.score_type;
        printf("  Detected Event : ");
        if (current_type != SCORE_TYPE_NONE) {
             printf("\033[1;31m%s\033[0m\n", get_score_type_str(current_type)); // Red
        } else {
             printf("%s\n", get_score_type_str(current_type));
        }

        // Score Display
        printf("  Safety Score   : \033[1;36m%.2f / 100.0\033[0m \n", 
               g_p_shm->generated_info.driving_score.total_score); // Cyan
        
        printf("\n===================================================\n");
        printf(" UI Refresh Rate: 10Hz (Precision Mode) \n");

        // --- 2. Calculate Next Wake-up Time ---
        timespec_add_ns(&next_refresh, UI_REFRESH_PERIOD_NS);

        // --- 3. Precision Sleep ---
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_refresh, NULL);
    }

    detach_shared_memory(g_p_shm);
    printf("\n[Reader] Disconnected.\n");
    return 0;
}