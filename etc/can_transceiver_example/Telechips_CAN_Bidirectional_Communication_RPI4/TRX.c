#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>

/* ============================== */
/*           DEFINES              */
/* ============================== */

#define CAN_INTERFACE_NAME       "can0"
#define CAN_TX_ID_APU_TO_MCU     0x123
#define CAN_RX_ID_MCU_1          0x456
#define CAN_RX_ID_MCU_2          0x789

#define TX_PERIOD_US             500000   // 500 ms
#define CAN_MAX_DATA_LEN         8

/* ============================== */
/*        FUNCTION PROTOTYPES     */
/* ============================== */

/* Initialize SocketCAN and bind to can interface */
static int InitCanSocket(void);

/* Receive CAN frame from MCU */
static void ProcessCanRx(int i_sock_fd);

/* Transmit CAN frame to MCU */
static void SendCanTx(int i_sock_fd);

/* ============================== */
/*              MAIN              */
/* ============================== */

int main(void)
{
    int i_sock_fd = -1;

    /* Initialize CAN socket */
    i_sock_fd = InitCanSocket();
    if (i_sock_fd < 0)
    {
        return -1;
    }

    printf("APU CAN START\n");

    while (1)
    {
        ProcessCanRx(i_sock_fd);
        SendCanTx(i_sock_fd);

        usleep(TX_PERIOD_US);
    }

    close(i_sock_fd);
    return 0;
}

/* ============================== */
/*        FUNCTION DEFINITIONS    */
/* ============================== */

/*
 * Initialize SocketCAN RAW socket
 * Input  : None
 * Output : socket file descriptor (>=0 success, <0 fail)
 */
static int InitCanSocket(void)
{
    int i_sock_fd = -1;
    int i_flags = 0;

    struct sockaddr_can s_addr;
    struct ifreq s_ifr;

    /* Create CAN RAW socket */
    i_sock_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (i_sock_fd < 0)
    {
        perror("socket");
        return -1;
    }

    /* Get interface index */
    strcpy(s_ifr.ifr_name, CAN_INTERFACE_NAME);
    if (ioctl(i_sock_fd, SIOCGIFINDEX, &s_ifr) < 0)
    {
        perror("ioctl");
        close(i_sock_fd);
        return -1;
    }

    /* Bind socket to CAN interface */
    s_addr.can_family  = AF_CAN;
    s_addr.can_ifindex = s_ifr.ifr_ifindex;

    if (bind(i_sock_fd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        perror("bind");
        close(i_sock_fd);
        return -1;
    }

    /* Set socket to non-blocking mode */
    i_flags = fcntl(i_sock_fd, F_GETFL, 0);
    fcntl(i_sock_fd, F_SETFL, i_flags | O_NONBLOCK);

    return i_sock_fd;
}

/*
 * Receive CAN frame from MCU
 * Input  : socket file descriptor
 * Output : None
 */
static void ProcessCanRx(int i_sock_fd)
{
    int i_rx_bytes = 0;
    uint32_t u_can_id = 0;

    struct can_frame s_can_rx_frame;

    i_rx_bytes = read(i_sock_fd, &s_can_rx_frame, sizeof(struct can_frame));
    if (i_rx_bytes > 0)
    {
        /* Extract standard CAN ID */
        u_can_id = s_can_rx_frame.can_id & CAN_EFF_MASK;

        if ((u_can_id == CAN_RX_ID_MCU_1) || (u_can_id == CAN_RX_ID_MCU_2))
        {
            /* Make payload string-safe */
            if (s_can_rx_frame.can_dlc < CAN_MAX_DATA_LEN)
            {
                s_can_rx_frame.data[s_can_rx_frame.can_dlc] = '\0';
            }
            else
            {
                s_can_rx_frame.data[CAN_MAX_DATA_LEN - 1] = '\0';
            }

            printf("[MCU ---> APU] [0x%03X] %s\n\n",
                   u_can_id,
                   s_can_rx_frame.data);
        }
    }
}

/*
 * Transmit CAN frame to MCU
 * Input  : socket file descriptor
 * Output : None
 */
static void SendCanTx(int i_sock_fd)
{
    struct can_frame s_can_tx_frame;

    memset(&s_can_tx_frame, 0, sizeof(struct can_frame));

    s_can_tx_frame.can_id  = CAN_TX_ID_APU_TO_MCU;
    s_can_tx_frame.can_dlc = 6;   // "HI MCU"
    memcpy(s_can_tx_frame.data, "HI MCU", 6);

    if (write(i_sock_fd, &s_can_tx_frame, sizeof(struct can_frame)) < 0)
    {
        perror("write");
    }
    else
    {
        printf("[APU ---> MCU] [0x%03X] HI MCU\n\n", CAN_TX_ID_APU_TO_MCU);
    }
}