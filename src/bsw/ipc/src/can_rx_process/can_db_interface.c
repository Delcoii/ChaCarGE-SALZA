#include "can_db_interface.h"
#include <stdio.h>
#include <string.h>


IMUAccel SetIMUDataFromCAN(const struct can_frame *frame) {
    // TODO : make imu parsing
    IMUAccel a;
    return a;
}



VehicleCommand SetVehicleCommandFromCAN(const struct can_frame *frame) {	
	VehicleCommand vehicle_command;
    const uint8_t *d = frame->data;

    vehicle_command.throttle = (double)((d[1] << 8) | d[0]) / 100;
    vehicle_command.brake    = (double)((d[3] << 8) | d[2]) / 100;
    // TODO : add offset
    vehicle_command.steer_tire_degree = (double)((d[5] << 8) | d[4]) / 100;
    
	return vehicle_command;
}

