#ifndef __VEHICLE_CONTROL_H__
#define __VEHICLE_CONTROL_H__

#include "main.h"
#include "cmsis_os.h"

#include "struct_remote_signal.h"
#include "struct_vehicle_command.h"

#define MIN_PULSE_WIDTH_US      1000    // detected pulse width min
#define NEUTRAL_PULSE_WIDTH_US  1500    // detected pulse width neutral
#define MAX_PULSE_WIDTH_US      2000    // detected pulse width max
#define PULSE_DETECT_ERR_OFFSET 100     // detected pulse width error offset

#define MAX_STEER_TIRE_DEG      20.09
#define MAX_THROTTLE            100.0

#define KP                      0
#define KI                      0
#define KD                      0

typedef enum {
    ERROR_MODE = 0,
    MANUAL_MODE,
    AUTO_MODE,
    BRAKE_MODE,
} VehicleMode_e;


double PIDControl(double ref, double sense);
double LinearMapping(double x, double in_min, double in_max, double out_min, double out_max);


VehicleCommand_t PulseToVehicleCommand(RemoteSignals_t remote);


void StopMotor(void);
void MoveForward(double pedal, uint32_t arr);
void MoveBackward(double pedal, uint32_t arr);
void MoveSteer(double steer_tire_deg, uint32_t arr);


#endif // __VEHICLE_CONTROL_H__