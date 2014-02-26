#include <msp430.h> 
#include "common.h"
#include "device.h"
#include "data.h"

inline
void INIT()
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    // MCLK and SMCLK set to 1MHZ
    DCOCTL = CALDCO_16MHZ;
	BCSCTL1 = CALBC1_16MHZ;
}

/*
 * main.c
 */
int main(void)
{
	INIT();

	for (;;)
	{
		uint16_t evt = Disptch_OneWire_Events();
#ifdef __IMPORT_SUPPORT
		switch (evt)
		{
		case EVT_IMPORT_ROM:
			ImportData();
			break;

		default:
			break;
		}
#endif
	}

	return 0;
}
