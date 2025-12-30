#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h> // -lm need for compilation
#include <string.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>


#include "shm_functions.h"
#include "can_db_interface.h"

// global variable (for signal handler)
ShmIntegrated* p_shm = NULL;
int keep_running = 1;
static int can_socket = -1;

// shutdown signal handler
void signal_handler(int sig) {
    printf("\n[Generator] Test data generator shutdown requested. Cleaning up...\n");
    keep_running = 0;
}

// can
static int init_can_socket(const char *ifname);


int main() {
    // 1. enroll signal handler
    signal(SIGINT, signal_handler);

    // 2. initialize
    printf("[Generator] Test data generator starting...\n");
    p_shm = init_shared_memory();
    if (p_shm == NULL)
    {
        fprintf(stderr, "[Generator - Error] SHM initialization failed.\n");
        return 1;
    }

    // init can
    if (init_can_socket("can0") != 0) {
        printf("can init failed!");
    }
    struct can_frame frame;     // received can frame

    
    // initialize variables
    double time_step = 0.0;

    printf("[Generator] Test data generation started (Ctrl+C to stop)\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("  TIME  |  THROTTLE  |   BRAKE   |  STEER  | TRAFFIC |  IMU_X  |  IMU_Y  |  IMU_Z  |  DRIVE_DISTANCE \n");
    printf("--------------------------------------------------------------------------------------------------------\n");

    uint32_t check_id;
    // 3. main loop (data generation -> shared memory write)
    while (keep_running) {   
        int nbytes = read(can_socket, &frame, sizeof(frame));
        if (nbytes < 0) {
            if (errno == EINTR) continue;
            perror("read(can)");
            continue;
        }
        if (nbytes != sizeof(frame)) {
            fprintf(stderr, "Incomplete CAN frame (%d bytes)\n", nbytes);
            continue;
        }

        // check error
        if (frame.can_id & CAN_ERR_FLAG) continue; 
        if (frame.can_id & CAN_RTR_FLAG) continue; 
        check_id = frame.can_id & CAN_EFF_MASK;
        if (!(frame.can_id & CAN_EFF_FLAG)) {
            check_id &= CAN_SFF_MASK;
        }
        // printf("RX ID: 0x%X, Data: %02X %02X %02X %02X %02X %02X %02X %02X\n", 
        //        check_id, 
        //        frame.data[0], frame.data[1], frame.data[2], frame.data[3],
        //        frame.data[4], frame.data[5], frame.data[6], frame.data[7]);

        // TODO : mutex?
        switch (check_id) {
            case CANID_VEHICLE_COMMAND1:
                p_shm->given_info.vehicle_command = SetVehicleCommandFromCAN(&frame);
                break;
            case CANID_IMU_DATA:
                p_shm->given_info.imu_accel = SetIMUDataFromCAN(&frame);
                break;
            case CANID_TRAFFIC_SIGN:
                p_shm->given_info.traffic_state = SetTrafficSignFromCAN(&frame);
                break;    
            default:
                break;
        }  
       
    
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
        // usleep(100000); // 100ms (10Hz)
    }

    // 4. shutdown processing
    printf("\n[Generator] shutting down.\n");
    detach_shared_memory(p_shm);
    destroy_shared_memory(); // if this process shutting down, destroy SHM

    return 0;
}


static int init_can_socket(const char *ifname) {
    struct sockaddr_can addr;
    struct ifreq ifr;
    
    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) { perror("socket(PF_CAN)"); return -1; }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl(SIOCGIFINDEX)");
        close(can_socket);
        can_socket = -1;
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind(AF_CAN)");
        close(can_socket);
        can_socket = -1;
        return -1;
    }

    struct can_filter rfilter[4];
    rfilter[0].can_id   = CANID_VEHICLE_COMMAND1;
    rfilter[0].can_mask = CAN_SFF_MASK;  // 11-bit
    
    rfilter[1].can_id   = CANID_VEHICLE_COMMAND2;
    rfilter[1].can_mask = CAN_SFF_MASK;

    rfilter[2].can_id   = CANID_IMU_DATA;
    rfilter[2].can_mask = CAN_SFF_MASK;

    rfilter[3].can_id   = CANID_TRAFFIC_SIGN;
    rfilter[3].can_mask = CAN_SFF_MASK;

    if (setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_FILTER,
                   &rfilter, sizeof(rfilter)) < 0) {
        perror("setsockopt(CAN_RAW_FILTER)");
    }

    printf("CAN Socket bound on %s (ifindex=%d)\n", ifname, ifr.ifr_ifindex);
    return 0;
}
