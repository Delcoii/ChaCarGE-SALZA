#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>


#include <linux/can.h>
#include <linux/can/raw.h>

// ===== Shared Memory  =====
#define SHM_NAME "/shm_traffic_sign"

// ===== CAN setting Channel, ID ======
#define CAN_IFACE "can0"
#define CAN_ID 0x123


// ====== Shared memory struct ======
typedef struct {
    uint8_t traffic_state;
}ShmGiveninfo;

typedef struct {
    ShmGiveninfo given_info;
} ShmIntegrated;



// shared memory open 
static ShmIntegrated* open_shared_memory(void) 
{
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("[CAN_TX] shm_open");
        return NULL;
    }

    void* ptr = mmap(0, sizeof(ShmIntegrated), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("[CAN_TX] mmap");
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);
    return (ShmIntegrated*)ptr;
}

// SocketCAN Open 
static int open_can_socket(void) 
{
    int soc = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (soc < 0) {
        perror("[CAN_TX] socket");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, CAN_IFACE, IFNAMSIZ - 1);

    if (ioctl(soc, SIOCGIFINDEX, &ifr) < 0) {
        perror("[CAN_TX] ioctl(SIOCGIFINDEX)");
        close(soc);
        return -1;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[CAN_TX] bind");
        close(soc);
        return -1;
    }

    return soc;
}


int main(void)
{
	// shared memory open
    ShmIntegrated* shm = open_shared_memory();
    if (!shm){
	    return 1;
    }

    int can_fd = open_can_socket();
    if (can_fd < 0) {
	    return 1;
    }

    printf("[CAN_TX] start, iface=%s id=0x%X\n", CAN_IFACE, CAN_ID);
	
    // main loop
    while (1) {
        uint8_t state = shm->given_info.traffic_state;

        struct can_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.can_id  = CAN_ID;
        frame.can_dlc = 1;
        frame.data[0] = state;

        ssize_t w = write(can_fd, &frame, sizeof(frame));
        if (w != (ssize_t)sizeof(frame)) {
            fprintf(stderr, "[CAN_TX] write failed: %s\n", strerror(errno));
        }

        usleep(100000); // period : 100ms
    }

    return 0;
}
