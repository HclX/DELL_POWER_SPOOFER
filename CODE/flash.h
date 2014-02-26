/*
 * flash.h
 *
 *  Created on: Feb 16, 2014
 *      Author: xuan_000
 */

#ifndef FLASH_H_
#define FLASH_H_
#include "common.h"

bool FlashErase(void* blk);
bool FlashWrite(void* dst, const void* src, uint16_t size);


#endif /* FLASH_H_ */
