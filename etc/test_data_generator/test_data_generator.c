#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h> // -lm need for compilation
#include <string.h>

#include "shm_functions.h"

// global variable (for signal handler)
ShmIntegrated* p_shm = NULL;
int keep_running = 1;

// shutdown signal handler
void signal_handler(int sig)
{
    printf("\n[Generator] Test data generator shutdown requested. Cleaning up...\n");
    keep_running = 0;
}

int main()
{
    // 1. enroll signal handler
    signal(SIGINT, signal_handler);

    // 2. initialize shared memory
    printf("[Generator] Test data generator starting...\n");
    p_shm = init_shared_memory();
    if (p_shm == NULL)
    {
        fprintf(stderr, "[Generator - Error] SHM initialization failed.\n");
        return 1;
    }

    // initialize variables
    double time_step = 0.0;
    int traffic_timer = 0;

    printf("[Generator] Test data generation started (Ctrl+C to stop)\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("  TIME  |  THROTTLE  |   BRAKE   |  STEER  | TRAFFIC |  IMU_X  |  IMU_Y  |  IMU_Z  |  DRIVE_DISTANCE \n");
    printf("--------------------------------------------------------------------------------------------------------\n");

    // 3. main loop (data generation -> shared memory write)
    while (keep_running)
    {   
        // --- [A] IMU accel generation (vibration and gravity) ---
        // path: p_shm -> given_info (ShmGivenInfo) -> imu_accel
        p_shm->given_info.imu_accel.x_mps2 = 0.2 * sin(time_step);       
        p_shm->given_info.imu_accel.y_mps2 = 9.8 + (0.05 * cos(time_step * 5)); 
        p_shm->given_info.imu_accel.z_mps2 = 0.1 * sin(time_step * 2);

        // --- [B] vehicle command (accel -> constant -> brake pattern) ---
        // repeating every 20 seconds
        double cycle = fmod(time_step, 20.0);
        
        if (cycle < 8.0 && p_shm->given_info.traffic_state.sign_state != TRAFFIC_STATE_RED)
        { 
            // 0~8sec: accel section
            p_shm->given_info.vehicle_command.throttle = cycle * 10.0; // 0 ~ 80
            if(p_shm->given_info.vehicle_command.throttle > 100) p_shm->given_info.vehicle_command.throttle = 100;
            p_shm->given_info.vehicle_command.brake = 0;
        } 
        else if (cycle < 15.0 && p_shm->given_info.traffic_state.sign_state != TRAFFIC_STATE_RED)
        {
            // 8~15sec: constant speed (no throttle, coasting)
            p_shm->given_info.vehicle_command.throttle = 30; 
            p_shm->given_info.vehicle_command.brake = 0;
        }
        else
        {
            // 15~20sec: deceleration section
            p_shm->given_info.vehicle_command.throttle = 0;
            p_shm->given_info.vehicle_command.brake = (cycle - 15.0) * 15.0; // 0 ~ 75
        }

        // --- [C] handle steering (left and right)
        p_shm->given_info.vehicle_command.steer_tire_degree = 10.0 * sin(time_step * 0.5);

        // --- [D] shuffling traffic state (5sec) ---
        traffic_timer++;
        if (traffic_timer % 50 == 0)
        { // 0.1s * 50 = 5초
            uint32_t current = p_shm->given_info.traffic_state.sign_state;
            // 0(Red) -> 1(Yellow) -> 2(Green) -> 0...
            p_shm->given_info.traffic_state.sign_state = (current + 1) % 4; 
        }

        // --- [E] drive distance ---
        p_shm->given_info.drive_distance.data_km += 0.0005; 

        // --- [F] monitoring output ---
        printf("\r  %4.1fs |    %3.0f     |    %3.0f    |  %5.1f  |    %d    |  %4.2f  |  %4.2f  |  %4.2f  |  %4.2f  ", 
               time_step,
               p_shm->given_info.vehicle_command.throttle,
               p_shm->given_info.vehicle_command.brake,
               p_shm->given_info.vehicle_command.steer_tire_degree,
               p_shm->given_info.traffic_state.sign_state,
               p_shm->given_info.imu_accel.x_mps2,
               p_shm->given_info.imu_accel.y_mps2,
               p_shm->given_info.imu_accel.z_mps2,
               p_shm->given_info.drive_distance.data_km
               );
        
        fflush(stdout); // clearing buffer

        // updating time
        time_step += 0.1;
        usleep(100000); // 100ms (10Hz)
    }

    // 4. shutdown processing
    printf("\n[Generator] shutting down.\n");
    detach_shared_memory(p_shm);
    destroy_shared_memory(); // if this process shutting down, destroy SHM

    return 0;
}