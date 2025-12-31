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

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "shm_layout.h"

// CAN ID Definition
#define CANID_TRAFFIC_SIGN      0x123
#define CANID_VEHICLE_COMMAND1	0x150
#define CANID_VEHICLE_COMMAND2	0x151

#define CANID_REMOTE_SIGNALS	0x160

#define CANID_STEER_ADC		    0x170

#define CANID_IMU_DATA			0x507
//#define CANID_ADC_DATA			0x444	// Test ADC Message


IMUAccel SetIMUDataFromCAN(const struct can_frame *frame);
VehicleCommand SetVehicleCommandFromCAN(const struct can_frame *frame);
TrafficSignState SetTrafficSignFromCAN(const struct can_frame *frame);

#endif // __CAN_DB_INTERFACE_H__
