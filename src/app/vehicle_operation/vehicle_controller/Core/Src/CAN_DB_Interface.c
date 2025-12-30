/*
 * CAN_DB_Interface.c
 *
 *  Created on: Dec 28, 2025
 *      Author: user
 */

#include "can_db_interface.h"
#include <stdio.h>
#include <string.h>


void SetRemoteSignalsCANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory) {
	// clear all 8 bytes to avoid sending data
	memset(p_frame->data, 0 , sizeof(p_frame->data));

	p_frame->RemoteSignalsFrame_t.steering_pulse_width_us
		= (uint16_t)(shared_memory.remote.steering_pulse_width_us);
	p_frame->RemoteSignalsFrame_t.throttle_pulse_width_us
	    = (uint16_t)(shared_memory.remote.throttle_pulse_width_us);
	p_frame->RemoteSignalsFrame_t.toggle_pulse_width_us
	    = (uint16_t)(shared_memory.remote.toggle_pulse_width_us);
	p_frame->RemoteSignalsFrame_t.mode_pulse_width_us
	    = (uint16_t)(shared_memory.remote.mode_pulse_width_us);
}

void SetSteerADCCANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory) {
	// clear all 8 bytes to avoid sending data
	memset(p_frame->data, 0 , sizeof(p_frame->data));

	p_frame->ADCFrame_t.data = (uint16_t)(shared_memory.steer_adc);
}


void SetVehicleCommand1CANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory) {
	// clear all 8 bytes to avoid sending data
	memset(p_frame->data, 0 , sizeof(p_frame->data));

	p_frame->VehicleCommand1Frame_t.throttle
		= (uint16_t)(shared_memory.vehicle_command.throttle * 100);

	p_frame->VehicleCommand1Frame_t.brake
		= (uint16_t)(shared_memory.vehicle_command.brake * 100);

	p_frame->VehicleCommand1Frame_t.steer_tire_degree
		= (uint16_t)((shared_memory.vehicle_command.steer_tire_degree + 50.0) * 100.0);

	p_frame->VehicleCommand1Frame_t.steer_adc = (uint16_t)(shared_memory.vehicle_command.steer_adc);
}



void SetVehicleCommand2CANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory) {
	// clear all 8 bytes to avoid sending data
	memset(p_frame->data, 0 , sizeof(p_frame->data));

	p_frame->VehicleCommand2Frame_t.mode = (uint8_t)(shared_memory.vehicle_command.mode);
	p_frame->VehicleCommand2Frame_t.toggle = (uint8_t)(shared_memory.vehicle_command.toggle);
}

void SetIMUData1CANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory) {
	// clear all 8 bytes to avoid sending data
	memset(p_frame->data, 0 , sizeof(p_frame->data));

	p_frame->IMUData1Frame_t.acc_x_mps2
		= (uint16_t)((shared_memory.imu_data.acc_x_mps2 + 20.0) * 100.0);
	p_frame->IMUData1Frame_t.acc_y_mps2
		= (uint16_t)((shared_memory.imu_data.acc_y_mps2 + 20.0) * 100.0);
	p_frame->IMUData1Frame_t.acc_z_mps2
		= (uint16_t)((shared_memory.imu_data.acc_z_mps2 + 20.0) * 100.0);
}

void SetIMUData2CANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory) {
	// clear all 8 bytes to avoid sending data
	memset(p_frame->data, 0 , sizeof(p_frame->data));

	p_frame->IMUData2Frame_t.gyro_x_dps
		= (uint16_t)((shared_memory.imu_data.gyro_x_dps + 720.0) * 100.0);
	p_frame->IMUData2Frame_t.gyro_y_dps
		= (uint16_t)((shared_memory.imu_data.gyro_y_dps + 720.0) * 100.0);
	p_frame->IMUData2Frame_t.gyro_z_dps
		= (uint16_t)((shared_memory.imu_data.gyro_z_dps + 720.0) * 100.0);
}


void SetIMUData3CANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory) {
	// clear all 8 bytes to avoid sending data
	memset(p_frame->data, 0 , sizeof(p_frame->data));

	p_frame->IMUData3Frame_t.roll_deg
		= (uint16_t)((shared_memory.imu_data.roll_deg + 720.0) * 100.0);
	p_frame->IMUData3Frame_t.pitch_deg
		= (uint16_t)((shared_memory.imu_data.pitch_deg + 720.0) * 100.0);
	p_frame->IMUData3Frame_t.yaw_deg
		= (uint16_t)((shared_memory.imu_data.yaw_deg + 720.0) * 100.0);
}