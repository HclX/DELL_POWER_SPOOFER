#include <msp430.h> 
#include "common.h"
#include "device.h"
#include "data.h"

inline
void INIT()
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

#if CPU_FREQ == 16
    // MCLK and SMCLK set to 1MHZ
    DCOCTL = CALDCO_16MHZ;
	BCSCTL1 = CALBC1_16MHZ;
#elif CPU_FREQ == 8
    // MCLK and SMCLK set to 1MHZ
    DCOCTL = CALDCO_8MHZ;
	BCSCTL1 = CALBC1_8MHZ;
#elif CPU_FREQ == 1
    // MCLK and SMCLK set to 1MHZ
    DCOCTL = CALDCO_1MHZ;
	BCSCTL1 = CALBC1_1MHZ;
#endif
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
