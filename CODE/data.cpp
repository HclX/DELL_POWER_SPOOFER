/*
 * data.cpp
 *
 *  Created on: Feb 17, 2014
 *      Author: xuan_000
 */



#include "data.h"
#include "flash.h"

#define DATA_AREA_START	0xFC00
#define DATA_AREA_SIZE	0x200

const uint8_t _data[] =
{
		// ROM ID
		0x11, 0x63, 0x4d, 0x8B, 0x00, 0x00, 0x00, 0x14,

		// MEM
		0x44, 0x45, 0x4C, 0x4C, 0x30, 0x30, 0x41, 0x43, 0x30, 0x39, 0x30, 0x31, 0x39, 0x35, 0x30, 0x34,
		0x36, 0x43, 0x4E, 0x30, 0x39, 0x54, 0x32, 0x31, 0x35, 0x37, 0x31, 0x36, 0x31, 0x35, 0x34, 0x33,
		0x38, 0x33, 0x35, 0x45, 0x41, 0x4C, 0x30, 0x33, 0xE0, 0xA9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

#ifdef __IMPORT_SUPPORT

inline
void pullUp()
{
	P1DIR &= ~MASTER_LINE;
	P1REN |= MASTER_LINE;
	P1OUT |= MASTER_LINE;
}

inline
void pullDown()
{
	P1DIR |= MASTER_LINE;
	P1OUT &= ~MASTER_LINE;
	P1REN &= ~MASTER_LINE;
}

inline
bool isBusHigh()
{
	return P1IN & MASTER_LINE;
}

static bool Reset()
{
	pullUp();
	DELAY_US(10);

	pullDown();
	DELAY_US(480);

	pullUp();
	DELAY_US(60);

	if (isBusHigh())
	{
		return false;
	}

	DELAY_US(300);
	return isBusHigh();
}

static void Write(uint8_t data)
{
	pullUp();
	DELAY_US(5);

	for (int i = 0; i < 8; i ++)
	{
		DELAY_US(10);
		pullDown();

		if (data & 0x01)
		{
			DELAY_US(5);
			pullUp();
			DELAY_US(55);
		}
		else
		{
			DELAY_US(60);
			pullUp();
		}

		data >>= 1;
	}
}

static uint8_t Read()
{
	uint8_t data = 0;
	uint8_t bit = 1;
	for (int i = 0; i < 8; i ++)
	{
		DELAY_US(10);

		pullDown();
		DELAY_US(1);

		pullUp();
		DELAY_US(5);

		if (isBusHigh())
		{
			data |= bit;
		}

		bit <<= 1;
		DELAY_US(60);
	}

	return data;
}

static bool ReadRom(uint8_t* out)
{
	// Initialize the bus
	if (!Reset())
	{
		return false;
	}

	//READ ROM command
	Write(0x33);

	// Calculating CRC while receiving the data
	uint8_t crc = 0;
	for (uint8_t i = 0; i < ROM_SIZE; i ++)
	{
		out[i] = Read();
		crc = CRC_8(crc, out[i]);
	}

	// CRC should be zero for a successful read
	return crc == 0;
}

static bool ReadMem(uint8_t* out)
{
	if (!Reset())
	{
		return false;
	}

	// Skip ROM
	Write(0xCC);

	uint8_t crc = 0;
	crc = CRC_8(crc, 0xF0);

	// ReadMem command
	Write(0xF0);

	crc = CRC_8(crc, 0x00);
	// Low address
	Write(0x00);

	crc = CRC_8(crc, 0x00);
	// High address
	Write(0x00);

	// Acknowledge
	uint8_t ack = Read();
	crc = CRC_8(crc, ack);

	if (crc != 0)
	{
		return false;
	}

	crc = 0;
	for (uint16_t i = 0; i < MEM_SIZE; i ++)
	{
		out[i] = Read();
		crc = CRC_8(crc, out[i]);
	}

	ack = Read();
	crc = CRC_8(crc, ack);

	return crc == 0;
}

bool ImportData()
{
	uint8_t data[DATA_SIZE];

	if (!ReadRom(data))
	{
		return false;
	}

	if (!ReadMem(data + ROM_SIZE))
	{
		return false;
	}

	FlashErase((void*)DATA_AREA_START);
	FlashWrite((void*)DATA_AREA_START, data, sizeof(data));

	return 0 == memcmp(data, _data, sizeof(data));
}

#endif
