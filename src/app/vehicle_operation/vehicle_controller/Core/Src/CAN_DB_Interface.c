/*
 * CAN_DB_Interface.c
 *
 *  Created on: Dec 28, 2025
 *      Author: user
 */

#include "CAN_DB_Interface.h"
#include <stdio.h>
#include <string.h>

void CAN_SetVehicleCommand(CAN_VEHICLE_COMMAND_t *frame, double steer_deg, double accel_per, double brake_per)
{
	// clear all 8 bytes to avoid sending data
	memset(frame->data, 0 , sizeof(frame->data));


	//scaling
	frame->RAW.steer_raw = clamp_int16((int32_t)(steer_deg * 100));	// deg
	frame->RAW.accel_raw = clamp_u8(accel_per);		// %
	frame->RAW.brake_raw = clamp_u8(brake_per);		// %

	// frame->RAW.remote_throttle_us / steer_adc_raw are set by the caller (TaskCANTX)
}


void CAN_SetIMUData(CAN_IMU_DATA_t *frame, double ax, double ay, double az)
{
	// clear all 8 bytes to avoid sending data
	memset(frame->data, 0, sizeof(frame->data));

	//scaling
	frame->RAW.ax_raw = clamp_int16((int32_t)(ax * 100.0));
	frame->RAW.ay_raw = clamp_int16((int32_t)(ay * 100.0));
	frame->RAW.az_raw = clamp_int16((int32_t)(az * 100.0));

		// frame->RAW.remote_steer_us is set by the caller (TaskCANTX)
}


