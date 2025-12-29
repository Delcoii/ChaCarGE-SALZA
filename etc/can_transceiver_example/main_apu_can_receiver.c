#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include "shm_layout.h"
#include "CAN_DB_Interface.h"

#define CANID_VEHICLE_COMMAND   0x150
#define CANID_IMU_DATA          0x507

static ShmIntegrated *shm_ptr = NULL;
static pthread_mutex_t shm_mutex = PTHREAD_MUTEX_INITIALIZER;
static int can_socket = -1;

static int init_shared_memory(void)
{
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) { perror("shm_open"); return -1; }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        close(shm_fd);
        return -1;
    }

    shm_ptr = (ShmIntegrated*)mmap(NULL, SHM_SIZE,
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        shm_ptr = NULL;
        return -1;
    }

    // 여기 memset은 “항상 초기화”라서,
    // 다른 프로세스랑 공유 중이면 데이터 날릴 수 있음.
    // 단일 프로세스 실험이면 Ok
    memset(shm_ptr, 0, SHM_SIZE);

    close(shm_fd);
    printf("Shared Memory initialized\n");
    return 0;
}

static int init_can_socket(const char *ifname)
{
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

    // (중요) 필요한 ID만 수신하도록 필터 적용
    struct can_filter rfilter[2];
    rfilter[0].can_id   = CANID_VEHICLE_COMMAND;
    rfilter[0].can_mask = CAN_SFF_MASK;  // 11-bit
    rfilter[1].can_id   = CANID_IMU_DATA;
    rfilter[1].can_mask = CAN_SFF_MASK;

    if (setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_FILTER,
                   &rfilter, sizeof(rfilter)) < 0) {
        perror("setsockopt(CAN_RAW_FILTER)");
        // 필터 실패해도 수신은 되니까 return은 안 해도 됨
    }

    printf("CAN Socket bound on %s (ifindex=%d)\n", ifname, ifr.ifr_ifindex);
    return 0;
}


static inline int16_t le_i16(uint8_t lo, uint8_t hi) {
    return (int16_t)((hi << 8) | lo);
}

static void parse_vehicle_command_frame(const struct can_frame *frame)
{
    if (frame->can_dlc != 8) return;

    const uint8_t *d = frame->data;

    //  스케일은 MCU CAN_DB랑 반드시 맞춰야 함
    // (예: steer_raw = steer_deg * 100 이면 0.01로 복원)
    int16_t steer_raw = le_i16(d[0], d[1]);
    double steer_deg  = steer_raw * 0.01;      // <-- 여기 (기존 0.1이면 틀어질 수 있음)

    double throttle   = d[2];                  // 0~100
    double brake      = d[3];                  // 0~100

    pthread_mutex_lock(&shm_mutex);
    shm_ptr->given_info.vehicle_command.steer_tire_degree = steer_deg;
    shm_ptr->given_info.vehicle_command.throttle = throttle;
    shm_ptr->given_info.vehicle_command.brake = brake;
    pthread_mutex_unlock(&shm_mutex);

    printf("[RX 0x150] steer=%.2f deg, thr=%.1f%%, brk=%.1f%%\n",
           steer_deg, throttle, brake);
}

static void parse_imu_frame(const struct can_frame *frame)
{
    if (frame->can_dlc != 8) return;

    const uint8_t *d = frame->data;

    int16_t ax_raw = le_i16(d[0], d[1]);
    int16_t ay_raw = le_i16(d[2], d[3]);
    int16_t az_raw = le_i16(d[4], d[5]);

    // 보통 0.001 m/s^2 단위로 스케일링하면 이런 복원이 맞음
    double ax = ax_raw * 0.001;
    double ay = ay_raw * 0.001;
    double az = az_raw * 0.001;

    pthread_mutex_lock(&shm_mutex);
    shm_ptr->given_info.imu_accel.x_mps2 = ax;
    shm_ptr->given_info.imu_accel.y_mps2 = ay;
    shm_ptr->given_info.imu_accel.z_mps2 = az;
    pthread_mutex_unlock(&shm_mutex);

    printf("[RX 0x507] ax=%.3f ay=%.3f az=%.3f (m/s^2)\n", ax, ay, az);
}

static void *can_receiver_thread(void *arg)
{
    (void)arg;
    struct can_frame frame;

    while (1) {
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

        //  flag 제거 후 ID 추출
        if (frame.can_id & CAN_ERR_FLAG) continue; // 에러프레임 무시
        if (frame.can_id & CAN_RTR_FLAG) continue; // RTR 무시(데이터 없음)

        uint32_t id = frame.can_id & CAN_EFF_MASK; // 표준/확장 모두 커버

        if (!(frame.can_id & CAN_EFF_FLAG)) {
            // standard frame => 실제 id는 11-bit
            id &= CAN_SFF_MASK;
        }

        switch (id) {
            case CANID_VEHICLE_COMMAND: parse_vehicle_command_frame(&frame); break;
            case CANID_IMU_DATA:        parse_imu_frame(&frame); break;
            default:
                // 필터 걸어놨으면 여기 거의 안 옴
                break;
        }
    }
    return NULL;
}

int main(void)
{
    pthread_t th;

    if (init_shared_memory() != 0)
    {
        return 1;
    } 
    
    if (init_can_socket("can0") != 0)
    {
        return 1;
    } 

    if (pthread_create(&th, NULL, can_receiver_thread, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    printf("APU CAN Receiver started\n");
    pthread_join(th, NULL);

    munmap(shm_ptr, SHM_SIZE);
    close(can_socket);
    return 0;
}
