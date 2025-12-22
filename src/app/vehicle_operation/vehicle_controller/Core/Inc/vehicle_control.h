#ifndef __VEHICLE_CONTROL_H__
#define __VEHICLE_CONTROL_H__

#include "main.h"
#include "cmsis_os.h"


#define MAX_STEER_TIRE_DEG      20.09
#define MIN_PULSE_WIDTH_US      1000    // detected pulse width min
#define MAX_PULSE_WIDTH_US      2000    // detected pulse width max


typedef struct {
    double throttle;            // 0.0 ~ 100.0
    double brake;               // 0.0 ~ 100.0
    double steer_tire_degree;  // -1.0 ~ 1.0 * (MAX_STEER_TIRE_DEG)
} VehicleCommand_t;



double PIDControl(double ref, double sense, double dt_us);
double LinearMapping(double x, double in_min, double in_max, double out_min, double out_max);


#endif // __VEHICLE_CONTROL_H__