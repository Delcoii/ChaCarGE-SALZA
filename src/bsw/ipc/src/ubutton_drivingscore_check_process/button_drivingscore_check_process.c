#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <wiringPi.h>

#include "shm_functions.h"

#define BUTTON_PIN 24          // BCM GPIO24
#define POLL_INTERVAL_MS 20    // simple debounce / poll interval

static volatile sig_atomic_t keep_running = 1;
static ShmIntegrated *p_shm = NULL;

static void handle_sigint(int sig) {
    (void)sig;
    keep_running = 0;
}

static int init_gpio(void) {
    if (wiringPiSetupGpio() == -1) {
        perror("[Button] wiringPiSetupGpio failed");
        return -1;
    }

    pinMode(BUTTON_PIN, INPUT);
    pullUpDnControl(BUTTON_PIN, PUD_UP); // default high, active-low button
    return 0;
}

int main(void) {
    signal(SIGINT, handle_sigint);

    if (init_gpio() != 0) {
        return 1;
    }

    p_shm = init_shared_memory();
    if (p_shm == NULL) {
        fprintf(stderr, "[Button] Failed to initialize shared memory.\n");
        return 1;
    }

    // start with driving score check disabled
    p_shm->given_info.bUseDrivingScoreChecking = 0;
    printf("[Button] bUseDrivingScoreChecking initialized to 0 (off)\n");

    int prev_state = digitalRead(BUTTON_PIN);

    while (keep_running) {
        int curr_state = digitalRead(BUTTON_PIN);

        // Detect falling edge (button press if pulled up)
        if (prev_state == HIGH && curr_state == LOW) {
            uint8_t new_val = p_shm->given_info.bUseDrivingScoreChecking ? 0 : 1;
            p_shm->given_info.bUseDrivingScoreChecking = new_val;
            printf("[Button] Toggled bUseDrivingScoreChecking -> %u\n", new_val);
            fflush(stdout);
        }

        prev_state = curr_state;
        delay(POLL_INTERVAL_MS);
    }

    printf("[Button] Exiting, cleaning up...\n");
    detach_shared_memory(p_shm);
    return 0;
}
