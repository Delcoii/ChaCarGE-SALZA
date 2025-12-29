#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h> 
#include <string.h>
#include <stdint.h>      // [추가] uint64_t 사용을 위해 필요
#include <sys/timerfd.h> // [추가] timerfd 사용을 위해 필요

#include "shm_functions.h"

// 전역 변수 (시그널 핸들러용)
// typedef로 정의했으므로 'struct' 키워드 생략
ShmIntegrated* p_shm = NULL;
int keep_running = 1;
int timer_fd = -1; // [추가] 타이머 파일 디스크립터

void signal_handler(int sig)
{
    printf("\n[Generator] 종료 요청을 받았습니다. 정리 중...\n");
    keep_running = 0;
}

int main()
{
    // 1. 시그널 등록
    signal(SIGINT, signal_handler);

    // 2. 공유 메모리 초기화
    printf("[Generator] 공유 메모리 연결 중...\n");
    p_shm = init_shared_memory();
    if (p_shm == NULL)
    {
        fprintf(stderr, "[Error] 공유 메모리 초기화 실패.\n");
        return 1;
    }

    // ---------------------------------------------------------
    // [변경점 1] TimerFD 설정 (usleep 대체)
    // ---------------------------------------------------------
    
    // (1) 타이머 생성 (CLOCK_MONOTONIC: 절대 시간)
    timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1) {
        perror("[Error] timerfd_create failed");
        return 1;
    }

    // (2) 시간 설정 구조체
    struct itimerspec ts;
    
    // 주기 (Interval): 100ms = 100,000,000 ns
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 100000000; 

    // 초기 시작 (Value): 설정 후 즉시 시작할지 여부 (여기선 주기와 동일하게 설정)
    ts.it_value.tv_sec = 0;
    ts.it_value.tv_nsec = 100000000;

    // (3) 타이머 시작
    if (timerfd_settime(timer_fd, 0, &ts, NULL) == -1) {
        perror("[Error] timerfd_settime failed");
        close(timer_fd);
        return 1;
    }
    // ---------------------------------------------------------

    // 변수 초기화
    double time_step = 0.0;
    int traffic_timer = 0;
    uint64_t expirations = 0; // 타이머가 몇 번 울렸는지 담을 변수

    printf("[Generator] 데이터 생성 시작 (TimerFD 모드, 100ms 주기)\n");
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("  TIME  |  THROTTLE  |   BRAKE   |  STEER  | TRAFFIC |  IMU_X  |  IMU_Y  |  IMU_Z  |  DRIVE_DISTANCE \n");
    printf("--------------------------------------------------------------------------------------------------------\n");

    // 3. 메인 루프
    while (keep_running)
    {   
        // --- [A] ~ [E] 데이터 생성 로직 (기존과 동일) ---

        // IMU 가속도 (진동 효과)
        p_shm->given_info.imu_accel.x_mps2 = 0.2 * sin(time_step);       
        p_shm->given_info.imu_accel.y_mps2 = 9.8 + (0.05 * cos(time_step * 5)); 
        p_shm->given_info.imu_accel.z_mps2 = 0.1 * sin(time_step * 2);

        // 차량 명령 (가속-정속-감속 패턴)
        double cycle = fmod(time_step, 20.0);
        if (cycle < 8.0 && p_shm->given_info.traffic_state.sign_state != 1) { // 빨간불 아닐 때 가속
            p_shm->given_info.vehicle_command.throttle = cycle * 10.0;
            if(p_shm->given_info.vehicle_command.throttle > 100) p_shm->given_info.vehicle_command.throttle = 100;
            p_shm->given_info.vehicle_command.brake = 0;
        } else if (cycle < 15.0 && p_shm->given_info.traffic_state.sign_state != 1) { // 정속
            p_shm->given_info.vehicle_command.throttle = 30; 
            p_shm->given_info.vehicle_command.brake = 0;
        } else {
            p_shm->given_info.vehicle_command.throttle = 0;
            p_shm->given_info.vehicle_command.brake = (cycle - 15.0) * 15.0;
        }

        // 조향각
        p_shm->given_info.vehicle_command.steer_tire_degree = 10.0 * sin(time_step * 0.5);

        // 신호등 (5초 주기)
        traffic_timer++;
        if (traffic_timer % 50 == 0) {
            uint32_t current = p_shm->given_info.traffic_state.sign_state;
            p_shm->given_info.traffic_state.sign_state = (current + 1) % 4; 
        }

        // 주행거리
        p_shm->given_info.drive_distance.data_km += 0.0005; 

        // --- [F] 모니터링 출력 ---
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

        // ---------------------------------------------------------
        // [변경점 2] 정확한 주기 대기 (read 사용)
        // ---------------------------------------------------------
        // 타이머가 울릴 때까지 대기 (Blocking)
        // 작업이 빨리 끝나면 남은 시간만큼 쉬고, 늦게 끝나면 바로 넘어감(밀림 방지)
        int ret = read(timer_fd, &expirations, sizeof(expirations));
        
        if (ret == -1) {
            perror("read timer");
            break;
        }

        // 시간 업데이트
        time_step += 0.1;
    }

    // 4. 종료 처리
    printf("\n[Generator] 종료합니다.\n");
    
    // [변경점 3] 타이머 닫기
    if (timer_fd != -1) close(timer_fd);

    detach_shared_memory(p_shm);
    destroy_shared_memory();

    return 0;
}