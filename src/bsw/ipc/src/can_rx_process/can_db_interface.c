#include "can_db_interface.h"
#include <stdio.h>
#include <string.h>


IMUAccel SetIMUDataFromCAN(const struct can_frame *frame) {
    // TODO : make imu parsing
    IMUAccel accel_data;
    memset(&accel_data, 0, sizeof(IMUAccel));
    const uint8_t *d = frame->data;

    int16_t raw_x = (int16_t)((d[1] << 8) | d[0]);
    int16_t raw_y = (int16_t)((d[3] << 8) | d[2]);
    int16_t raw_z = (int16_t)((d[5] << 8) | d[4]);

    accel_data.x_mps2 = (double)raw_x / 100.0 - 20;
    accel_data.y_mps2 = (double)raw_y / 100.0 - 20;
    accel_data.z_mps2 = (double)raw_z / 100.0 - 20;

    return accel_data;
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
    vehicle_command.steer_tire_degree = (double)raw_steer / 100.0 - 50;
    
    
	return vehicle_command;
}

TrafficSignState SetTrafficSignFromCAN(const struct can_frame *frame) {
    TrafficSignState traffic_sign;
    
    const uint8_t *d = frame->data;
    const uint8_t raw_state = d[0];
    // Assuming sign_state is in the first byte
    
    switch(raw_state){
        case 0:
            traffic_sign.sign_state = TRAFFIC_STATE_NONE;
            break;
        case 1:
            traffic_sign.sign_state = TRAFFIC_STATE_RED;
            break;
        case 2:
            traffic_sign.sign_state = TRAFFIC_STATE_YELLOW;
            break;
        case 3:
            traffic_sign.sign_state = TRAFFIC_STATE_GREEN;
            break;
        default:
            break;
    }
    
    return traffic_sign;
}


