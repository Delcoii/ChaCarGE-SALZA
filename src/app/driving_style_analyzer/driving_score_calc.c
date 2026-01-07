#include <stdio.h>
#include <math.h> // Requires -lm option
#include <string.h>
#include "driving_score_calc.h"
// #include "struct_traffic_sign.h" // For TRAFFIC_STATE_RED

// Initialize Algorithm State
void init_algo_state(AlgoState* state) {
    memset(state, 0, sizeof(AlgoState));
    state->is_first_loop = 1;

    // MA Buffer Init
    state->bias_acc_z = -9.80665; // Assuming stationary at start

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
    double r_brake  = (double)seg->hard_brake_ticks / total;
    double r_accel  = (double)seg->sudden_accel_ticks / total;
    
    // Count of Events
    double count_turn   = (double)seg->aggressive_turn_ticks;
    double count_bump   = (double)seg->bump_ticks;
    double count_signal = (double)seg->signal_violation_ticks;

    // Calculate PCF (Base * Factor ^ Ratio)
    double pcf = PCF_BASE;
    pcf *= pow(FACTOR_HARD_BRAKING, r_brake);
    pcf *= pow(FACTOR_SUDDEN_ACCEL, r_accel);
    pcf *= pow(FACTOR_AGGRESSIVE_TURNING, count_turn);
    pcf *= pow(FACTOR_SPEED_BUMP, count_bump);
    pcf *= pow(FACTOR_SIGNAL_VIOLATION, count_signal);

    // Convert to Score (0 ~ 100)
    double score = SCORE_INTERCEPT - (SCORE_SLOPE * pcf);

    if (score > 100.0) score = 100.0;
    if (score < 0.0) score = 0.0;

    return score;
}

// [Main Logic] Update Driving Score & Detect Events
void update_driving_score(const ShmGivenInfo* input, ShmGeneratedInfo* output, AlgoState* state) {
    state->event_detected = 0; // Reset event flag at the beginning of the cycle
    
    // 1. Calculate Mileage (Delta Distance)
    double current_dist = input->drive_distance.data_km;
    double delta_dist = 0.0;
    
    // 2. Sensor Data Extraction
    //double acc_x = input->imu_accel.x_mps2;
    //double acc_y = input->imu_accel.y_mps2;
    double raw_acc_z = input->imu_accel.z_mps2;
    uint8_t traffic = input->traffic_state.sign_state;
    double throttle = input->vehicle_command.throttle;
    double steer = input->vehicle_command.steer_tire_degree;

    if (state->is_first_loop) {
        state->prev_total_distance_km = current_dist;
        state->prev_traffic_state = traffic;
        state->prev_throttle = throttle;

        // Initialize cooldown timers
        state->bump_cooldown_ticks = 0;
        state->signal_cooldown_ticks = 0;
        state->sudden_curve_cooldown_ticks = 0;
        state->sudden_accel_cooldown_ticks = 0;

        // Initialize bias
        state->bias_acc_z = raw_acc_z; // Calibrate bias
        state->prev_acc_z = state->bias_acc_z;

        // Initialize MA Buffer with first value
        for(int i=0; i<MAX_WINDOW_SIZE; ++i){
            state->acc_z_buffer[i] = raw_acc_z;
        }
        state->acc_z_sum = raw_acc_z * MAX_WINDOW_SIZE;
        state->acc_z_idx = 0;
        state->is_buffer_full = 1;

        state->is_first_loop = 0;
    }
    // else {
    //     delta_dist = current_dist - state->prev_total_distance_km;
    //     if (delta_dist < 0) {
    //         delta_dist = 0.0;
    //     }

    //     // for testing does distance really need
    //     if (delta_dist == 0)
    //     {
    //         delta_dist = 0.00001;
    //     } 
    // }
    state->prev_total_distance_km = current_dist;

    // 3. Update Segment Data
    ScoreSegment* curr_seg = &state->history[state->current_seg_idx];
    curr_seg->distance_accumulated_km += delta_dist;
    curr_seg->total_ticks++; // +1 tick (10ms)

    // [AccZ Moving Average filter]
    // Subtract old value
    state->acc_z_sum -= state->acc_z_buffer[state->acc_z_idx];
    // Add new value
    state->acc_z_buffer[state->acc_z_idx] = raw_acc_z;
    state->acc_z_sum += raw_acc_z;
    // Move index
    state->acc_z_idx = (state->acc_z_idx + 1) % MAX_WINDOW_SIZE;

    double filtered_acc_z = state->acc_z_sum / (double)MAX_WINDOW_SIZE;
    // double filtered_acc_z = raw_acc_z * LPF_ALPHA + state->prev_acc_z * (1.0 - LPF_ALPHA);
    // state->prev_acc_z = filtered_acc_z; // Update previous value for next iteration

    // [B] Throttle Rate Calculation (% per sec)
    // dt = 0.01s (10ms)
    double throttle_rate = (throttle - state->prev_throttle) / 0.01;

    // [C] Event Cooldown Timers
    if(state->bump_cooldown_ticks > 0) {
        state->bump_cooldown_ticks--;
    }
    if(state->signal_cooldown_ticks > 0) {
        state->signal_cooldown_ticks--;
    }
    if(state->sudden_curve_cooldown_ticks > 0) {
        state->sudden_curve_cooldown_ticks--;
    }
    if(state->sudden_accel_cooldown_ticks > 0) {
        state->sudden_accel_cooldown_ticks--;
    }

    // 4. Detect Risky Behaviors & Update 'score_type'
    // Priority: Signal Violation > Sudden Curve > Sudden Accel/Brake > None
    // We check in reverse order of priority or use else-if carefully.
    
    // output->driving_score_type.score_type = SCORE_TYPE_NONE; // Reset first
    // output->driving_score_type.count = 0;

    // (A) Signal Violation (Highest Priority)
    if(!state->signal_cooldown_ticks) {
        if (state->prev_traffic_state == TRAFFIC_STATE_RED && traffic == TRAFFIC_STATE_NONE && state->prev_throttle > 70.0) {
            curr_seg->signal_violation_ticks++;
            output->driving_score_type.count = ++state->signal_violation_count;
            output->driving_score_type.score_type = SCORE_IGNORE_SIGN;
            state->event_detected = 1;
            state->signal_cooldown_ticks = SUDDEN_EVENT_COOLDOWN_TICKS; // Set cooldown
        }
    }

    /*Countinuos Events Check*/
    int is_turning = (throttle > FULL_THROTTLE_LIMIT && fabs(steer) > MAX_STEER_ANGLE);
    int is_accel = (throttle_rate > THRESH_THROTTLE_RATE_PPS && throttle > MIN_THROTTLE_FOR_ACCEL);
    //int is_brake = (acc_x < -THRESH_BRAKE_MPS2);
    
    if (!state->event_detected) {
        // (B) Sudden Curve (If no higher priority event)
        if(is_turning || is_accel/* || is_brake*/) {
        
            if(is_turning && !state->sudden_curve_cooldown_ticks) {
                curr_seg->aggressive_turn_ticks++;
                output->driving_score_type.count = ++state->sudden_curve_count;
                output->driving_score_type.score_type = SCORE_SUDDEN_CURVE;
                state->sudden_curve_cooldown_ticks = SUDDEN_EVENT_COOLDOWN_TICKS; // Set cooldown
            }
    
            // (C) Sudden Accel (Mapped to Sudden Accel Type)    
            else if(is_accel && !state->sudden_accel_cooldown_ticks) {
                curr_seg->sudden_accel_ticks++;
                output->driving_score_type.count = ++state->sudden_accel_count;
                output->driving_score_type.score_type = SCORE_SUDDEN_ACCEL; 
                state->sudden_accel_cooldown_ticks = SUDDEN_EVENT_COOLDOWN_TICKS; // Set cooldown
            }
                    
        }        // (D) Speed Bump Detection (If no higher priority event)
        // Note: Bump detection is independent of continuous events
        else {
            if(state->bump_cooldown_ticks == 0 && (filtered_acc_z) >= (-2.0) ) {
                curr_seg->bump_ticks++;
                output->driving_score_type.score_type = SCORE_BUMP;
                output->driving_score_type.count = ++state->bump_count;
                
                state->bump_cooldown_ticks = EVENT_COOLDOWN_TICKS; // Set cooldown
            }

           // All checks done for continuous events
        }
    

        
    
    }
    // Update state for next loop
    state->prev_traffic_state = traffic;
    state->prev_throttle = throttle;

    // 5. Calculate Real-time Score
    curr_seg->score = calculate_tesla_score(curr_seg);

    // 6. Check Segment Completion (TMAP Logic)
    // if (curr_seg->distance_accumulated_km >= SEGMENT_DISTANCE_KM) {
    if (curr_seg->total_ticks >= SEGMENT_TIMEOUT_TICKS) {
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

    output->total_score = (valid_count > 0) ? (total_sum / valid_count) : 100.0;
}