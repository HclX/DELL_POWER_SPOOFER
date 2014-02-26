/*
 * common.h
 *
 *  Created on: Feb 8, 2014
 *      Author: xuan_000
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <msp430.h>


#define CPU_FREQ 		16

#ifndef CALDCO_16MHZ
// Specifically calibrated for the MSP430G2211 chip I have
#define CALDCO_16MHZ	0xD6
#define CALBC1_16MHZ	0xBE
#endif

#define DELAY_US(x)		__delay_cycles(CPU_FREQ * (x))

//#define __FAST_CRC__

#ifdef __FAST_CRC__

extern const uint8_t __crc[];

inline
uint8_t CRC_8(uint8_t crc, uint8_t data)
{
	return __crc[crc ^ data];
}
#else
uint8_t CRC_8(uint8_t crc, uint8_t data);
#endif

#define MASTER_LINE		BIT1
#define SLAVE_LINE		BIT2

#define USER_BUTTON		BIT3
#define LED_PIN			BIT0

#endif /* COMMON_H_ */
