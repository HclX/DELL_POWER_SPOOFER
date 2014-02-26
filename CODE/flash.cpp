/*
 * flash.cpp
 *
 *  Created on: Feb 16, 2014
 *      Author: xuan_000
 */

#include "flash.h"

bool FlashErase(void* blk)
{
	if (blk == NULL)
	{
		return false;
	}

// Erase block first
	FCTL2 = FWKEY | FSSEL_2 | FN0;
	FCTL3 = FWKEY;

	FCTL1 = FWKEY | ERASE;

	(*(char*)blk) = 0x00;

	while (FCTL3 & BUSY);

	FCTL1 = FWKEY;
	FCTL3 = FWKEY | LOCK;
	return true;
}

bool FlashWrite(void* dst, const void* src, uint16_t cb)
{
// Copy memory
	FCTL1 = FWKEY | WRT;

	memcpy((uint8_t*)dst, (const uint8_t*)src, cb);

	while (FCTL3 & BUSY);

	FCTL1 = FWKEY;
	FCTL3 = FWKEY | LOCK;

	return true;
}


