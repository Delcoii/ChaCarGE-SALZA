#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>

// 사용자가 정의한 헤더들 (기존 유지)
#include "shm_functions.h"
#include "can_db_interface.h"

// =============================================================
// IPC Protocol Definitions
// =============================================================
#define IPC_SYNC        (0xFF)
#define IPC_START1      (0x55)
#define IPC_START2      (0xAA)
#define IPC_HDR_SIZE    (9)
#define IPC_MAX_PACKET  (0x400) // 1024 bytes
#define TCC_IPC_MAGIC   ('I')
// FLUSH는 사용하지 않을 것이므로 정의만 남겨둠 (혹은 삭제 가능)
#define IOCTL_IPC_FLUSH (_IO(TCC_IPC_MAGIC, 4)) 

// =============================================================
// Global Variables
// =============================================================
ShmIntegrated* p_shm = NULL;
int keep_running = 1;
static int ipc_fd = -1;

// =============================================================
// Signal Handler
// =============================================================
void signal_handler(int sig) {
    printf("\n[Generator] Shutdown requested. Cleaning up...\n");
    keep_running = 0;
}

// =============================================================
// CRC Function
// =============================================================
static uint16_t ipc_crc16(const uint8_t *buf, uint32_t len, uint16_t init) {
    static const uint16_t crc16Table[256] = {
        0x0000U, 0x1021U, 0x2042U, 0x3063U, 0x4084U, 0x50a5U, 0x60c6U, 0x70e7U,
        0x8108U, 0x9129U, 0xa14aU, 0xb16bU, 0xc18cU, 0xd1adU, 0xe1ceU, 0xf1efU,
        0x1231U, 0x0210U, 0x3273U, 0x2252U, 0x52b5U, 0x4294U, 0x72f7U, 0x62d6U,
        0x9339U, 0x8318U, 0xb37bU, 0xa35aU, 0xd3bdU, 0xc39cU, 0xf3ffU, 0xe3deU,
        0x2462U, 0x3443U, 0x0420U, 0x1401U, 0x64e6U, 0x74c7U, 0x44a4U, 0x5485U,
        0xa56aU, 0xb54bU, 0x8528U, 0x9509U, 0xe5eeU, 0xf5cfU, 0xc5acU, 0xd58dU,
        0x3653U, 0x2672U, 0x1611U, 0x0630U, 0x76d7U, 0x66f6U, 0x5695U, 0x46b4U,
        0xb75bU, 0xa77aU, 0x9719U, 0x8738U, 0xf7dfU, 0xe7feU, 0xd79dU, 0xc7bcU,
        0x48c4U, 0x58e5U, 0x6886U, 0x78a7U, 0x0840U, 0x1861U, 0x2802U, 0x3823U,
        0xc9ccU, 0xd9edU, 0xe98eU, 0xf9afU, 0x8948U, 0x9969U, 0xa90aU, 0xb92bU,
        0x5af5U, 0x4ad4U, 0x7ab7U, 0x6a96U, 0x1a71U, 0x0a50U, 0x3a33U, 0x2a12U,
        0xdbfdU, 0xcbdcU, 0xfbbfU, 0xeb9eU, 0x9b79U, 0x8b58U, 0xbb3bU, 0xab1aU,
        0x6ca6U, 0x7c87U, 0x4ce4U, 0x5cc5U, 0x2c22U, 0x3c03U, 0x0c60U, 0x1c41U,
        0xedaeU, 0xfd8fU, 0xcdecU, 0xddcdU, 0xad2aU, 0xbd0bU, 0x8d68U, 0x9d49U,
        0x7e97U, 0x6eb6U, 0x5ed5U, 0x4ef4U, 0x3e13U, 0x2e32U, 0x1e51U, 0x0e70U,
        0xff9fU, 0xefbeU, 0xdfddU, 0xcffcU, 0xbf1bU, 0xaf3aU, 0x9f59U, 0x8f78U,
        0x9188U, 0x81a9U, 0xb1caU, 0xa1ebU, 0xd10cU, 0xc12dU, 0xf14eU, 0xe16fU,
        0x1080U, 0x00a1U, 0x30c2U, 0x20e3U, 0x5004U, 0x4025U, 0x7046U, 0x6067U,
        0x83b9U, 0x9398U, 0xa3fbU, 0xb3daU, 0xc33dU, 0xd31cU, 0xe37fU, 0xf35eU,
        0x02b1U, 0x1290U, 0x22f3U, 0x32d2U, 0x4235U, 0x5214U, 0x6277U, 0x7256U,
        0xb5eaU, 0xa5cbU, 0x95a8U, 0x8589U, 0xf56eU, 0xe54fU, 0xd52cU, 0xc50dU,
        0x34e2U, 0x24c3U, 0x14a0U, 0x0481U, 0x7466U, 0x6447U, 0x5424U, 0x4405U,
        0xa7dbU, 0xb7faU, 0x8799U, 0x97b8U, 0xe75fU, 0xf77eU, 0xc71dU, 0xd73cU,
        0x26d3U, 0x36f2U, 0x0691U, 0x16b0U, 0x6657U, 0x7676U, 0x4615U, 0x5634U,
        0xd94cU, 0xc96dU, 0xf90eU, 0xe92fU, 0x99c8U, 0x89e9U, 0xb98aU, 0xa9abU,
        0x5844U, 0x4865U, 0x7806U, 0x6827U, 0x18c0U, 0x08e1U, 0x3882U, 0x28a3U,
        0xcb7dU, 0xdb5cU, 0xeb3fU, 0xfb1eU, 0x8bf9U, 0x9bd8U, 0xabbbU, 0xbb9aU,
        0x4a75U, 0x5a54U, 0x6a37U, 0x7a16U, 0x0af1U, 0x1ad0U, 0x2ab3U, 0x3a92U,
        0xfd2eU, 0xed0fU, 0xdd6cU, 0xcd4dU, 0xbdaaU, 0xad8bU, 0x9de8U, 0x8dc9U,
        0x7c26U, 0x6c07U, 0x5c64U, 0x4c45U, 0x3ca2U, 0x2c83U, 0x1ce0U, 0x0cc1U,
        0xef1fU, 0xff3eU, 0xcf5dU, 0xdf7cU, 0xaf9bU, 0xbfbaU, 0x8fd9U, 0x9ff8U,
        0x6e17U, 0x7e36U, 0x4e55U, 0x5e74U, 0x2e93U, 0x3eb2U, 0x0ed1U, 0x1ef0U
    };

    uint16_t crc = init;
    for (uint32_t i = 0; i < len; i++) {
        uint32_t temp = ((crc >> 8U) ^ buf[i]) & 0xFFU;
        crc = crc16Table[temp] ^ (uint16_t)(crc << 8U);
    }
    return crc;
}

// =============================================================
// Helper: Read Exact Bytes
// =============================================================
static int read_exact(int fd, uint8_t *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t r = read(fd, buf + got, len - got);
        if (r < 0) {
            if (errno == EINTR) continue;
            // Non-blocking 모드일 경우 데이터 없음
            if (errno == EAGAIN || errno == EWOULDBLOCK) return -2; 
            return -1;
        }
        if (r == 0) return -1; // EOF
        got += (size_t)r;
    }
    return 0;
}

// =============================================================
// IPC Read Function (Optimized)
// =============================================================
static int read_ipc_can_frame(int fd, struct can_frame *out_frame) {
    uint8_t hdr[IPC_HDR_SIZE];
    uint8_t data[IPC_MAX_PACKET];
    
    // [성능 개선] 1바이트씩 읽지 않고, 헤더 크기만큼 먼저 시도
    // 만약 스트림이 깨졌다면(Sync 안 맞음), 그때 1바이트씩 찾음
    
    // 1. 헤더 읽기 시도
    int ret = read_exact(fd, hdr, IPC_HDR_SIZE);
    if (ret == -2) return 0; // 데이터 없음 (Non-blocking)
    if (ret < 0) return -1;  // 에러

    // 2. Sync 체크 (Fast Path)
    if (hdr[0] == IPC_SYNC && hdr[1] == IPC_START1 && hdr[2] == IPC_START2) {
        // 정상: 다음 단계 진행
    } 
    else {
        // [복구 모드] Sync가 안 맞음. 1바이트씩 밀면서 Sync 찾기
        // 이미 읽은 hdr 버퍼를 활용해 sliding window 수행
        uint8_t window[3] = {hdr[IPC_HDR_SIZE-3], hdr[IPC_HDR_SIZE-2], hdr[IPC_HDR_SIZE-1]};
        // (간단하게 구현: 일단 동기화 맞을 때까지 1바이트씩 read)
        int synced = 0;
        // 주의: 이미 읽은 데이터 중 뒷부분에 sync가 있을 수 있으나, 
        // 복잡도 줄이기 위해 여기서부터 다시 찾음.
        
        while (!synced) {
             uint8_t b;
             if (read_exact(fd, &b, 1) != 0) return -1;
             window[0] = window[1];
             window[1] = window[2];
             window[2] = b;
             if (window[0] == IPC_SYNC && window[1] == IPC_START1 && window[2] == IPC_START2) {
                 // Sync 찾음! 헤더의 나머지 부분 읽기
                 hdr[0] = window[0]; hdr[1] = window[1]; hdr[2] = window[2];
                 if (read_exact(fd, &hdr[3], IPC_HDR_SIZE - 3) != 0) return -1;
                 synced = 1;
             }
        }
    }

    // 3. 데이터 길이 파싱
    uint16_t cmd2 = (uint16_t)((hdr[5] << 8) | hdr[6]);
    uint16_t len  = (uint16_t)((hdr[7] << 8) | hdr[8]);
    uint16_t rx_len = (len == 0) ? 1 : len;
    
    // 4. 데이터 본문 읽기
    if (read_exact(fd, data, rx_len + 2) != 0) return -1; // +2 for CRC

    // 5. CRC 검증
    uint16_t crc_calc = ipc_crc16(hdr, IPC_HDR_SIZE, 0);
    crc_calc = ipc_crc16(data, rx_len, crc_calc);
    uint16_t crc_recv = (uint16_t)((data[rx_len] << 8) | data[rx_len + 1]);

    if (crc_calc != crc_recv) {
        // CRC 에러 시 이번 프레임 무시하고 다음 프레임 대기
        return 0; 
    }

    // 6. 결과 출력
    memset(out_frame, 0, sizeof(*out_frame));
    out_frame->can_id = cmd2 & CAN_SFF_MASK;
    out_frame->can_dlc = (len > 8) ? 8 : len;
    if (out_frame->can_dlc > 0) {
        memcpy(out_frame->data, data, out_frame->can_dlc);
    }

    return 1; // 성공적으로 1개 읽음
}

static int init_ipc_device(const char *path) {
    ipc_fd = open(path, O_RDONLY | O_NONBLOCK) ; // Blocking 모드로 열기 (필요시 O_NONBLOCK 추가 가능)
    if (ipc_fd < 0) {
        perror("open(ipc)");
        return -1;
    }
    printf("IPC device opened: %s\n", path);
    return 0;
}


// =============================================================
// MAIN FUNCTION
// =============================================================
int main() {
    signal(SIGINT, signal_handler);

    printf("[Generator] Starting...\n");
    p_shm = init_shared_memory();
    if (p_shm == NULL) {
        fprintf(stderr, "[Error] SHM init failed.\n");
        return 1;
    }

    if (init_ipc_device("/dev/tcc_ipc_micom") != 0) {
        printf("IPC Init Failed!\n");
        return 1;
    }

    struct can_frame frame;
    double time_step = 0.0;
    uint32_t check_id;
    int print_counter = 0; // 출력 빈도 조절용

    printf("[Generator] Started. (Reading BUFFERED data)\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("  TIME  |  THROTTLE  |   BRAKE   |  STEER  | TRAFFIC |  IMU_X  |  IMU_Y  |  IMU_Z  |  DRIVE_DISTANCE \n");
    printf("--------------------------------------------------------------------------------------------------------\n");

    // =========================================================
    // MAIN LOOP
    // =========================================================
    while (keep_running) {
        
        // [중요] IOCTL_IPC_FLUSH 삭제함! (데이터 유실 방지)

        // [중요] Drain Loop: 커널 버퍼에 쌓인 데이터가 없을 때까지 계속 읽음
        // AP1이 보내는 데이터가 쌓여 있어도 여기서 순식간에 다 처리됨.
        int processed_count = 0;
        
        // Non-blocking 처럼 동작하게 하려면 O_NONBLOCK을 쓰거나, 
        // 여기서는 간단히 일정 횟수만 돌고 sleep 하도록 처리 (Starvation 방지)
        // 하지만 지금은 지연 해결이 목적이므로 buffer가 빌 때까지 읽는게 좋음.
        // *주의: read_exact가 Blocking이면 데이터가 올 때까지 기다림. 
        //        따라서 여기서는 1번 읽고, 데이터가 있으면 계속 읽는 방식.
        
        while (1) {
            // IPC로부터 1프레임 읽기
            int status = read_ipc_can_frame(ipc_fd, &frame);
            
            if (status < 0) {
                // 치명적 에러 (장치 끊김 등)
                perror("read error");
                keep_running = 0;
                break;
            }
            if (status == 0) {
                // 읽은 데이터가 없음 (혹은 CRC 에러로 무시됨) -> 루프 탈출하고 대기
                // *만약 Blocking 모드라면 여기서 대기하게 됨. 데이터 오면 바로 처리.
                // Blocking 모드에서는 while(1)로 계속 읽으면 sleep을 못하므로, 
                // AP1 데이터 폭주시 1회만 처리하고 넘어갈 수도 있음.
                // 하지만 지금은 '지연'이 문제이므로,
                // 아래 usleep을 줄이고 여기서 루프를 도는게 낫지는 않음 (Blocking일 경우).
                // **수정**: IPC 드라이버가 보통 Blocking이므로, 
                // 여기 while(1)을 쓰면 sleep 없이 100% CPU를 쓰며 실시간 처리함.
                // 일단 하나 읽었으니 처리하고, 다시 위로 올라가는 구조가 안전함.
                // 하지만 "지연"을 없애려면 모여있는걸 다 읽어야 함.
                // IPC 특성상 read()가 리턴했다는건 데이터가 있었다는 뜻.
                break; 
            }

            // 데이터 처리 (SHM 업데이트)
            check_id = frame.can_id & CAN_EFF_MASK;
            if (!(frame.can_id & CAN_EFF_FLAG)) {
                check_id &= CAN_SFF_MASK;
            }

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
            
            processed_count++;

            // [팁] 만약 AP1 데이터가 너무 빨라서 여기서 무한루프 돌 것 같으면
            // 안전장치로 100개 처리하면 강제로 break 해서 화면 갱신하게 함
            if (processed_count > 50) break; 
        }

        // --- 출력 (10번 루프 돌 때 1번만 출력) ---
        print_counter++;
        if (print_counter >= 10) {
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
            fflush(stdout); // 화면 갱신
            print_counter = 0;
            
            // 시간 업데이트 (출력 할 때만 증가시키는게 보기에 좋음, 혹은 실제 시간 사용)
            time_step += 0.1; 
        }

        // 너무 오래 쉬면 버퍼 쌓임. 100ms -> 10ms로 줄임. 
        // Blocking read라면 이 sleep은 큰 의미 없지만, CPU 점유율 낮추기 위함.
        usleep(10000); 
    }

    printf("\n[Generator] Shutting down.\n");
    detach_shared_memory(p_shm);
    destroy_shared_memory();
    if (ipc_fd >= 0) close(ipc_fd);

    return 0;
}