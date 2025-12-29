/*
 * CAN_DB_Interface.h
 *
 *  Created on: Dec 28, 2025
 *      Author: user
 */

#ifndef __CAN_DB_INTERFACE_H_
#define __CAN_DB_INTERFACE_H_

#include <stdint.h>
#include <string.h>

// CAN ID Definition
#define CANID_VEHICLE_COMMAND	0x150
#define CANID_IMU_DATA			0x507
//#define CANID_ADC_DATA			0x444	// Test ADC Message

//Test to check ADC Value
#pragma pack(push,1)

typedef union{
	uint8_t data[8];
	struct{
		uint16_t adc_raw;
		uint16_t adc_mv;
		uint8_t reserved[4];
	}RAW;
}CAN_ADC_DATA_t;
#pragma pack(pop)

// Vehicle Command Frame(DLC = 8)
// 0~1 : steer_raw (deg * 100)
// 2   : accel_raw (%)
// 3   : brake_raw (%)
// 4~5 : remote_throttle_us (raw pulse width)
// 6~7 : steer_adc_raw (raw ADC counts)
#pragma pack(push,1)

typedef union{
	uint8_t data[8];
	struct{
		int16_t steer_raw;
		uint8_t accel_raw;
		uint8_t brake_raw;
		uint16_t remote_throttle_us;
		uint16_t steer_adc_raw;
	}RAW;
}CAN_VEHICLE_COMMAND_t;

#pragma pack(pop)

//  IMU Data Frame (DLC = 8)
// 0~1 : ax_raw (m/s^2 * 100)
// 2~3 : ay_raw (m/s^2 * 100)
// 4~5 : az_raw (m/s^2 * 100)
// 6~7 : remote_steer_us (raw pulse width, optional debug)
#pragma pack(push, 1)
typedef union {
    uint8_t data[8];
    struct {
        int16_t ax_raw;
        int16_t ay_raw;
        int16_t az_raw;
        uint16_t remote_steer_us;
    }RAW;
} CAN_IMU_DATA_t;
#pragma pack(pop)


// Saturation Function(Vehicle_command, IMU)
static inline int16_t clamp_int16(int32_t v)
{
    if (v > 32767)  return 32767;
    if (v < -32768) return -32768;
    return (int16_t)v;
}

static inline uint8_t clamp_u8(double v)
{
    if (v < 0.0)   return 0;
    if (v > 100.0) return 100;
    return (uint8_t)v;
}


// Scaling Convert Function
void CAN_SetVehicleCommand(CAN_VEHICLE_COMMAND_t *frame, double steer_deg, double accel_per, double brake_per);
void CAN_SetIMUData(CAN_IMU_DATA_t *frame, double ax, double ay, double az);
void CAN_SetADCData(CAN_ADC_DATA_t *frame, double voltage);


// CAN Send Message
void CAN_Send_Message(uint16_t stdId, char *str);

#endif /* __CAN_DB_INTERFACE_H_ */
