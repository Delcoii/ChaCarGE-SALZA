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
    
    double score;    // Calculated score for this segment
    uint8_t is_valid;    // Flag indicating if the segment is completed/valid
} ScoreSegment;

// Overall Algorithm State (Previous distance, Segment history)
typedef struct {
    double prev_total_distance_km;
    ScoreSegment history[MAX_SEGMENT_HISTORY];
    uint8_t current_seg_idx; // Current segment index (0 ~ 5)
    uint8_t is_first_loop;   // Flag for the first loop execution
} AlgoState;

// Function Declarations
void init_algo_state(AlgoState* state);
void update_driving_score(const ShmGivenInfo* input, ShmGeneratedInfo* output, AlgoState* state);

#endif // DRIVING_SCORE_CALC_H