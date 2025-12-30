#ifndef DRIVING_SCORE_CONFIG_H
#define DRIVING_SCORE_CONFIG_H

/* --- 1. Tesla Safety Score PCF Constants --- */
#define PCF_BASE                    0.57198191

// Weight Factors
#define FACTOR_HARD_BRAKING         1.23599110
#define FACTOR_AGGRESSIVE_TURNING   1.01219290
#define FACTOR_SUDDEN_ACCEL         1.23599110
#define FACTOR_SIGNAL_VIOLATION     2.00000000
#define FACTOR_SPEED_BUMP           1.10000000 // bump factor

#define SCORE_INTERCEPT             122.15240383
#define SCORE_SLOPE                 38.72920381

/* --- 2. Physical Thresholds (Toy Car Optimized) --- */
#define GRAVITY_MPS2                9.80665

// Thresholds in g-force (Toy Car: 0.2g setting)
#define THRESH_HARD_BRAKE_G         0.20
#define THRESH_AGGRESSIVE_TURN_G    0.25
#define THRESH_SUDDEN_ACCEL_G       0.20
#define THRESH_BUMP_G               0.50  // bump detection threshold

// Automatic conversion to m/s^2
#define THRESH_BRAKE_MPS2           (THRESH_HARD_BRAKE_G * GRAVITY_MPS2)
#define THRESH_TURN_MPS2            (THRESH_AGGRESSIVE_TURN_G * GRAVITY_MPS2)
#define THRESH_ACCEL_MPS2           (THRESH_SUDDEN_ACCEL_G * GRAVITY_MPS2)
#define THRESH_BUMP_MPS2            (THRESH_BUMP_G * GRAVITY_MPS2)

/* --- 3. TMAP Segment Config --- */
#define SEGMENT_DISTANCE_KM         0.05   // 50m Segment
#define MAX_SEGMENT_HISTORY         6

#endif // DRIVING_SCORE_CONFIG_H