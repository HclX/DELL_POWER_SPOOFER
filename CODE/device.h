#ifndef __DS2502_H__
#define __DS2502_H__

#include "Common.h"

enum
{
	EVT_NONE,
	EVT_IMPORT_ROM
};

uint16_t Disptch_OneWire_Events();

#endif /* __DS2502_H__ */
