/*
 * CAN_DB_Interface.c
 *
 *  Created on: Dec 28, 2025
 *      Author: user
 */

#include "can_db_interface.h"
#include <stdio.h>
#include <string.h>


void SetRemoteSignalsCANFrame(VehicleCANFrame_t *frame, SharedMemory_t shared_memory) {
	// clear all 8 bytes to avoid sending data
	memset(frame->data, 0 , sizeof(frame->data));

	frame->RemoteSignalsFrame_t.steering_pulse_width_us
		= (uint16_t)(shared_memory.remote.steering_pulse_width_us);
	frame->RemoteSignalsFrame_t.throttle_pulse_width_us
	    = (uint16_t)(shared_memory.remote.throttle_pulse_width_us);
	frame->RemoteSignalsFrame_t.toggle_pulse_width_us
	    = (uint16_t)(shared_memory.remote.toggle_pulse_width_us);
	frame->RemoteSignalsFrame_t.mode_pulse_width_us
	    = (uint16_t)(shared_memory.remote.mode_pulse_width_us);
}



void SetVehicleCommand1CANFrame(VehicleCANFrame_t *frame, SharedMemory_t shared_memory) {
	// clear all 8 bytes to avoid sending data
	memset(frame->data, 0 , sizeof(frame->data));

	frame->VehicleCommand1Frame_t.throttle
		= (uint16_t)(shared_memory.vehicle_command.throttle * 100 + 0.5);

	frame->VehicleCommand1Frame_t.brake
		= (uint16_t)(shared_memory.vehicle_command.brake * 100 + 0.5);

	frame->VehicleCommand1Frame_t.steer_tire_degree
		= (uint16_t)(shared_memory.vehicle_command.steer_tire_degree * 100 + 0.5);

	frame->VehicleCommand1Frame_t.steer_adc = (uint16_t)(shared_memory.vehicle_command.steer_adc);
}



void SetVehicleCommand2CANFrame(VehicleCANFrame_t *frame, SharedMemory_t shared_memory) {
	// clear all 8 bytes to avoid sending data
	memset(frame->data, 0 , sizeof(frame->data));

	frame->VehicleCommand2Frame_t.mode = (uint8_t)(shared_memory.vehicle_command.mode);
	frame->VehicleCommand2Frame_t.toggle = (uint8_t)(shared_memory.vehicle_command.toggle);
}
