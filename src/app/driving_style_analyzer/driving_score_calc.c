#include <stdio.h>
#include <math.h> // Requires -lm option
#include <string.h>
#include "driving_score_calc.h"
// #include "struct_traffic_sign.h" // For TRAFFIC_STATE_RED

// Initialize Algorithm State
void init_algo_state(AlgoState* state) {
    memset(state, 0, sizeof(AlgoState));
    state->is_first_loop = 1;
    // Default score is 100 for all segments
    for(int i=0; i<MAX_SEGMENT_HISTORY; i++) {
        state->history[i].score = 100.0;
    }
}

// [Internal] Calculate Tesla Safety Score
static double calculate_tesla_score(const ScoreSegment* seg) {
    if (seg->total_ticks == 0) return 100.0;

    double total = (double)seg->total_ticks;
    
    // Calculate Ratios
    double r_bump   = (double)seg->bump_ticks / total;
    double r_brake  = (double)seg->hard_brake_ticks / total;
    double r_turn   = (double)seg->aggressive_turn_ticks / total;
    double r_accel  = (double)seg->sudden_accel_ticks / total;
    double r_signal = (double)seg->signal_violation_ticks / total;

    // Calculate PCF (Base * Factor ^ Ratio)
    double pcf = PCF_BASE;
    pcf *= pow(FACTOR_SPEED_BUMP, r_bump);
    pcf *= pow(FACTOR_HARD_BRAKING, r_brake);
    pcf *= pow(FACTOR_AGGRESSIVE_TURNING, r_turn);
    pcf *= pow(FACTOR_SUDDEN_ACCEL, r_accel);
    pcf *= pow(FACTOR_SIGNAL_VIOLATION, r_signal);

    // Convert to Score (0 ~ 100)
    double score = SCORE_INTERCEPT - (SCORE_SLOPE * pcf);

    if (score > 100.0) score = 100.0;
    if (score < 0.0) score = 0.0;

    return score;
}

// [Main Logic] Update Driving Score & Detect Events
void update_driving_score(const ShmGivenInfo* input, ShmGeneratedInfo* output, AlgoState* state) {
    
    // 1. Calculate Mileage (Delta Distance)
    double current_dist = input->drive_distance.data_km;
    double delta_dist = 0.0;

    if (state->is_first_loop) {
        state->prev_total_distance_km = current_dist;
        state->is_first_loop = 0;
    } else {
        delta_dist = current_dist - state->prev_total_distance_km;
        if (delta_dist < 0) delta_dist = 0; 
    }
    state->prev_total_distance_km = current_dist;

    // 2. Update Segment Data
    ScoreSegment* curr_seg = &state->history[state->current_seg_idx];
    curr_seg->distance_accumulated_km += delta_dist;
    curr_seg->total_ticks++; // +1 tick (10ms)

    // 3. Sensor Data Extraction
    double acc_x = input->imu_accel.x_mps2;
    double acc_y = input->imu_accel.y_mps2;
    double acc_z = input->imu_accel.z_mps2;
    uint8_t traffic = input->traffic_state.sign_state;
    double throttle = input->vehicle_command.throttle;

    // 4. Detect Risky Behaviors & Update 'score_type'
    // Priority: Signal Violation > Sudden Curve > Sudden Accel/Brake > None
    // We check in reverse order of priority or use else-if carefully.
    
    int event_detected = 0;
    output->driving_score.score_type = SCORE_TYPE_NONE; // Reset first

    // (A) Signal Violation (Highest Priority)
    if (traffic == TRAFFIC_STATE_RED && throttle > 0.0) {
        curr_seg->signal_violation_ticks++;
        output->driving_score.score_type = SCORE_IGNORE_SIGN;
        event_detected = 1;
    }
    
    // (B) Sudden Curve (If no higher priority event)
    if (!event_detected && fabs(acc_y) > THRESH_TURN_MPS2) {
        curr_seg->aggressive_turn_ticks++;
        output->driving_score.score_type = SCORE_SUDDEN_CURVE;
        event_detected = 1;
    }

    // (C) Speed Bump Detection (If no higher priority event)
    if(!event_detected && fabs(acc_z - GRAVITY_MPS2) > THRESH_BUMP_MPS2) {
        curr_seg->bump_ticks++;
        output->driving_score.score_type = SCORE_BUMP;
        event_detected = 1;
    }

    // (D) Sudden Accel and Hard Braking (Mapped to Sudden Accel Type)
    if (!event_detected && acc_x > THRESH_ACCEL_MPS2) {
        curr_seg->sudden_accel_ticks++;
        output->driving_score.score_type = SCORE_SUDDEN_ACCEL;
        event_detected = 1;
    }
    else if (!event_detected && acc_x < -THRESH_BRAKE_MPS2) {
        curr_seg->hard_brake_ticks++;
        output->driving_score.score_type = SCORE_SUDDEN_ACCEL; 
        event_detected = 1;
    }

    // 5. Calculate Real-time Score
    curr_seg->score = calculate_tesla_score(curr_seg);

    // 6. Check Segment Completion (TMAP Logic)
    if (curr_seg->distance_accumulated_km >= SEGMENT_DISTANCE_KM) {
        curr_seg->is_valid = 1;
        // printf("[Algo] Segment %d Finished. Score: %.2f\n", state->current_seg_idx, curr_seg->score);

        // Move to next segment
        state->current_seg_idx = (state->current_seg_idx + 1) % MAX_SEGMENT_HISTORY;
        
        // Reset next segment
        memset(&state->history[state->current_seg_idx], 0, sizeof(ScoreSegment));
        state->history[state->current_seg_idx].score = 100.0;
    }

    // 7. Update 'total_score' (The Final Output)
    // This value is written to Shared Memory every 10ms.
    // When the process terminates, the LAST written value remains here.
    double total_sum = 0;
    uint8_t valid_count = 0;
    
    for (int i=0; i<MAX_SEGMENT_HISTORY; i++) {
        if (state->history[i].total_ticks > 0 || state->history[i].is_valid) {
            total_sum += state->history[i].score;
            valid_count++;
        }
    }

    output->driving_score.total_score = (valid_count > 0) ? (total_sum / valid_count) : 100.0;
}