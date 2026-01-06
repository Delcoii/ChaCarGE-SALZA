#ifndef DRIVING_SCORE_CONFIG_H
#define DRIVING_SCORE_CONFIG_H

/* --- 1. Tesla Safety Score PCF Constants --- */
#define PCF_BASE                    0.57198191

// Weight Factors
#define FACTOR_HARD_BRAKING         1.23599110
#define FACTOR_AGGRESSIVE_TURNING   1.01219290
#define FACTOR_SUDDEN_ACCEL         1.23599110
#define FACTOR_SIGNAL_VIOLATION     1.32343362
#define FACTOR_SPEED_BUMP           1.02439511 // bump factor

#define SCORE_INTERCEPT             122.15240383
#define SCORE_SLOPE                 38.72920381

/* --- 2. Physical Thresholds (Toy Car Optimized) --- */
#define GRAVITY_MPS2                9.80665

// Thresholds in g-force (Toy Car: 0.2g setting)
#define THRESH_HARD_BRAKE_G         0.20
#define THRESH_AGGRESSIVE_TURN_G    0.25
#define THRESH_SUDDEN_ACCEL_G       0.20
#define THRESH_BUMP_G               0.70  // bump detection threshold

// Automatic conversion to m/s^2
#define THRESH_BRAKE_MPS2           (THRESH_HARD_BRAKE_G * GRAVITY_MPS2)
#define THRESH_BUMP_MPS2            (THRESH_BUMP_G * GRAVITY_MPS2)

// Base for Max values
#define MAX_STEER_ANGLE             19.0  // actual max steer angle is 20.0
#define FULL_THROTTLE_LIMIT         95.0  // actual full throttle is 100.0

// Base for Min values
#define MIN_THROTTLE_FOR_ACCEL      10.0  // minimum throttle to consider as acceleration

// Condition of sudden acceleration
#define THRESH_THROTTLE_RATE_PPS    700.0  // per second

/* --- 3. TMAP Segment Config --- */
#define SEGMENT_DISTANCE_KM         0.05   // 50m Segment
#define MAX_SEGMENT_HISTORY         6
#define SEGMENT_TIMEOUT_TICKS       6000   // 6000 ticks = 60s timeout

/* size of Moving Window */
#define MAX_WINDOW_SIZE             10
#define LPF_ALPHA                   0.1

// Cooldown for Bump Detection
#define BUMP_COOLDOWN_TICKS         50    // 50 ticks (0.5s)

#endif // DRIVING_SCORE_CONFIG_H