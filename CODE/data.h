/*
 * data.h
 *
 *  Created on: Feb 17, 2014
 *      Author: xuan_000
 */

#ifndef DATA_H_
#define DATA_H_

#include "common.h"

#define ROM_SIZE	0x08
#define MEM_SIZE	0x80
#define DATA_SIZE	(ROM_SIZE + MEM_SIZE)

extern const uint8_t _data[];

#ifdef __IMPORT_SUPPORT
bool ImportData();
#endif

#endif /* DATA_H_ */
