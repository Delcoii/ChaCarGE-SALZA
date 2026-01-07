#ifndef DRIVING_SCORE_CALC_H
#define DRIVING_SCORE_CALC_H

#include "shm_layout.h"
#include "driving_score_config.h"

// Data structure for a single driving segment
typedef struct {
    double distance_accumulated_km; // Accumulated distance in this segment
    
    // Time-based Measurements (Ticks)
    uint16_t total_ticks;           // Total driving time
    uint16_t hard_brake_ticks;      // Time spent hard braking
    uint16_t aggressive_turn_ticks; // Time spent aggressive turning
    uint16_t sudden_accel_ticks;    // Time spent sudden acceleration
    uint16_t signal_violation_ticks;// Time spent violating signals
    uint16_t bump_ticks;            // Time spent hitting bumps
    
    uint8_t is_valid;    // Flag indicating if the segment is completed/valid
    
    double score;    // Calculated score for this segment
} ScoreSegment;

// Overall Algorithm State (Previous distance, Segment history)
typedef struct {
    ScoreSegment history[MAX_SEGMENT_HISTORY];
    uint8_t current_seg_idx; // Current segment index (0 ~ 5)
    uint8_t is_first_loop;   // Flag for the first loop execution
    uint8_t event_detected;               // flag for any event detected in current tick
    double prev_total_distance_km;

    // Event Counts
    uint16_t sudden_accel_count;
    uint16_t sudden_curve_count;
    uint16_t bump_count;
    uint16_t signal_violation_count;

    // Moving Average for AccZ
    double acc_z_buffer[MAX_WINDOW_SIZE];    // buffer for AccZ values
    uint32_t acc_z_idx;                     // current index in the buffer
    uint32_t is_buffer_full;                // flag indicating if buffer is full
    double acc_z_sum;                       // sum of values in the buffer

    // Calibration AccZ
    double bias_acc_z;

    // For LPF prev AccZ
    double prev_acc_z;
    
    // Throttle Rate Calculation
    double prev_throttle;

    // Traffic Logic
    uint8_t prev_traffic_state;

    // State Variables for avoiding repeated event detection
    uint8_t is_continuous_event_active;     // flag for sudden curve/accel
    uint16_t bump_cooldown_ticks;           // cooldown timer for bump events
} AlgoState;

// Function Declarations
void init_algo_state(AlgoState* state);
void update_driving_score(const ShmGivenInfo* input, ShmGeneratedInfo* output, AlgoState* state);

#endif // DRIVING_SCORE_CALC_H