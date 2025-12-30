#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h> 
#include <string.h>
#include <stdint.h>
#include <sys/timerfd.h> // for using timerfd

#include "shm_functions.h"

// global (for signal handler)
ShmIntegrated* p_shm = NULL;
int keep_running = 1;
int timer_fd = -1;

void signal_handler(int sig)
{
    printf("\n[Generator] termination signal received.\n");
    keep_running = 0;
}

int main()
{
    // 1. signal enroll
    signal(SIGINT, signal_handler);

    // 2. shared memory initialize
    printf("[Generator] shared memory initializing...\n");
    p_shm = init_shared_memory();
    if (p_shm == NULL)
    {
        fprintf(stderr, "[Error] shared memory initialization failed.\n");
        return 1;
    }

    // ---------------------------------------------------------
    // TimerFD configueration
    // ---------------------------------------------------------
    
    // (1) create timer (CLOCK_MONOTONIC: by system uptime)
    timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1) {
        perror("[Error] timerfd_create failed");
        return 1;
    }

    // (2) time configue structure
    struct itimerspec ts;
    
    // Interval: 10ms = 10,000,000 ns
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 10000000; 

    // Initial Value: Whether to start immediately after configuration (Here, set equal to the period)
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 10000000;

    // (3) Timer start
    if (timerfd_settime(timer_fd, 0, &ts, NULL) == -1) {
        perror("[Error] timerfd_settime failed");
        close(timer_fd);
        return 1;
    }
    // ---------------------------------------------------------

    // Variables initialization
    double time_step = 0.0;
    int traffic_timer = 0;
    uint64_t expirations = 0; // how many timer expirations

    printf("[Generator] Data Generator Start (TimerFD mode, 10ms period)\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("  TIME  |  THROTTLE  |   BRAKE   |  STEER  | TRAFFIC |  IMU_X  |  IMU_Y  |  IMU_Z  |  DRIVE_DISTANCE \n");
    printf("--------------------------------------------------------------------------------------------------------\n");

    // 3. main loop
    while (keep_running)
    {   
        // --- [A] ~ [E] Data Generator Logic ---

        // IMU
        p_shm->given_info.imu_accel.x_mps2 = 0.2 * sin(time_step);       
        p_shm->given_info.imu_accel.y_mps2 = 9.8 + (0.05 * cos(time_step * 5)); 
        p_shm->given_info.imu_accel.z_mps2 = 0.1 * sin(time_step * 2);

        // Vehicle Command (Acceleration-Constant-Deceleration pattern)
        double cycle = fmod(time_step, 20.0);
        if (cycle < 8.0 && p_shm->given_info.traffic_state.sign_state != 1) { // Accel when not red light
            p_shm->given_info.vehicle_command.throttle = cycle * 10.0;
            if(p_shm->given_info.vehicle_command.throttle > 100) p_shm->given_info.vehicle_command.throttle = 100;
            p_shm->given_info.vehicle_command.brake = 0;
        } else if (cycle < 15.0 && p_shm->given_info.traffic_state.sign_state != 1) { // Constant
            p_shm->given_info.vehicle_command.throttle = 30; 
            p_shm->given_info.vehicle_command.brake = 0;
        } else {
            p_shm->given_info.vehicle_command.throttle = 0;
            p_shm->given_info.vehicle_command.brake = (cycle - 15.0) * 15.0;
        }

        // steer degree
        p_shm->given_info.vehicle_command.steer_tire_degree = 10.0 * sin(time_step * 0.5);

        // traffic signal (5sec period)
        traffic_timer++;
        if (traffic_timer % 500 == 0) {
            uint32_t current = p_shm->given_info.traffic_state.sign_state;
            p_shm->given_info.traffic_state.sign_state = (current + 1) % 4; 
        }

        // mileage
        p_shm->given_info.drive_distance.data_km += 0.0005; 

        // --- [F] Monitoring output ---
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
        fflush(stdout);

        // Wait for timer expiration (Blocking)
        // Compensates for execution time to maintain fixed frequency
        int ret = read(timer_fd, &expirations, sizeof(expirations));
        
        if (ret == -1) {
            perror("read timer");
            break;
        }

        // Time update
        time_step += 0.01;
    }

    // 4. end process
    printf("\n[Generator] end process.\n");
    
    // Timer close
    if (timer_fd != -1) close(timer_fd);

    detach_shared_memory(p_shm);
    destroy_shared_memory();

    return 0;
}