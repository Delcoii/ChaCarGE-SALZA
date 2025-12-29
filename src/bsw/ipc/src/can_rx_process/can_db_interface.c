#include "can_db_interface.h"
#include <stdio.h>
#include <string.h>


IMUAccel SetIMUDataFromCAN(const struct can_frame *frame) {
    // TODO : make imu parsing
    IMUAccel a;
    memset(&a, 0, sizeof(IMUAccel));
    return a;
}



VehicleCommand SetVehicleCommandFromCAN(const struct can_frame *frame) {	
	VehicleCommand vehicle_command;
    memset(&vehicle_command, 0, sizeof(VehicleCommand));
    const uint8_t *d = frame->data;

    // Little Endian Parsing & Signed 16-bit Cast
    int16_t raw_throttle = (int16_t)((d[1] << 8) | d[0]);
    int16_t raw_brake    = (int16_t)((d[3] << 8) | d[2]);
    int16_t raw_steer    = (int16_t)((d[5] << 8) | d[4]);

    vehicle_command.throttle = (double)raw_throttle / 100.0;
    vehicle_command.brake    = (double)raw_brake / 100.0;
    vehicle_command.steer_tire_degree = (double)raw_steer / 100.0;
    
    
	return vehicle_command;
}

