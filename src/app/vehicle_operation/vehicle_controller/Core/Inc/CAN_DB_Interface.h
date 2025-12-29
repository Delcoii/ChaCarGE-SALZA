/*
 * CAN_DB_Interface.h
 *
 *  Created on: Dec 28, 2025
 *      Author: user
 */

#ifndef __CAN_DB_INTERFACE_H__
#define __CAN_DB_INTERFACE_H__

#include <stdint.h>
#include <string.h>

#include "struct_shared_memory.h"

// CAN ID Definition
#define CANID_VEHICLE_COMMAND1	0x150
#define CANID_VEHICLE_COMMAND2	0x151

#define CANID_REMOTE_SIGNALS	0x160

#define CANID_STEER_ADC		    0x170

#define CANID_IMU_DATA			0x507
//#define CANID_ADC_DATA			0x444	// Test ADC Message


typedef union{
	uint8_t data[8];

	struct {
		uint16_t steering_pulse_width_us;
		uint16_t throttle_pulse_width_us;
		uint16_t toggle_pulse_width_us;
		uint16_t mode_pulse_width_us;
	} RemoteSignalsFrame_t;


	struct{
		uint16_t throttle;          // 2byte
		uint16_t brake;			    // 2byte
		uint16_t steer_tire_degree; // 2byte
		uint16_t steer_adc;         // 2byte
	} VehicleCommand1Frame_t;

	struct {
		uint8_t mode; 
		uint8_t toggle;
		uint8_t reserved[4];
	} VehicleCommand2Frame_t;

	struct {
		uint16_t data; 
		uint8_t reserved[4];
	} ADCFrame_t;


} VehicleCANFrame_t;



// Scaling Convert Function
void SetRemoteSignalsCANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory);
void SetSteerADCCANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory);
void SetVehicleCommand1CANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory);
void SetVehicleCommand2CANFrame(VehicleCANFrame_t *p_frame, SharedMemory_t shared_memory);

#endif // __CAN_DB_INTERFACE_H__
