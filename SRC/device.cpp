/*
 * OneWireClient.cpp
 *
 *  Created on: Feb 8, 2014
 *      Author: xuan_000
 */
#include "common.h"
#include "data.h"

enum
{
	S_INVALID	= 0x0000,	// expecting reset sequence
	S_ROM_CMD	= 0x0002,	// receiving ROM_CMD
	S_CMD		= 0x0004,	// receiving regular CMD
	S_ADDR_LB	= 0x0006,	// receiving low byte of address
	S_ADDR_HB	= 0x0008,	// receiving high byte of address
	S_ADDR_CRC  = 0x000A,	// sending address CRC_8
	S_ROM_READ	= 0x000C, 	// sending ROM bytes
	S_MEM_READ	= 0x000E,	// sending MEM bytes
	S_MEM_CRC	= 0x0010	// sending MEM CRC_8
};

static volatile uint16_t 	_xferBuf;
static volatile uint16_t 	_xferCnt;
static volatile uint16_t 	_addr;
static volatile uint16_t 	_crc;
static volatile uint16_t	_state;
static volatile uint16_t	_userEvent;

inline
void hold_line_low()
{
	P1DIR |= SLAVE_LINE;
	P1OUT &= ~SLAVE_LINE;
}

inline
void release_line()
{
	P1DIR &= ~SLAVE_LINE;
	P1REN &= ~SLAVE_LINE;
}

inline
bool is_line_high()
{
	return P1IN & SLAVE_LINE;
}

// The transfer buffer is designed so that we can correctly transfer
// the content without knowing if we are current sending or receiving by:
// 1. the lsb is always 0 for receiving mode
// 2. if the lsb is 0, no matter if it's receiving mode or sending mode,
//    we don't need to do anything
// 3. the line needs to be pulled low only if the lsb is 1. Since in the
//    protocol, sending 1 not zero requires pulling line low, we are
//    inverting the sending buffer when putting a byte there
// 4. _xferCnt to remember how many bits we received/transfered, and we
//    will handle the data once all 8 bits are processed before breaking
//    #1 & #2
inline
void set_xfer_buf(uint8_t data)
{
	_xferCnt = 8;
	_xferBuf = ~data;
}

inline
void clr_xfer_buf()
{
	_xferCnt = 8;
	_xferBuf = 0x0000;
}

inline
void set_ccr1_timer(uint16_t cycles)
{
	TACCTL1 = CCIE;
	TACCR1 = TAR + cycles;
}

// this happens either when a byte is sent or received, or if a reset sequence is finished
static void OnXferDone()
{
	switch (_state)
	{
	// this event at this state indicates we've done a reset sequence
	case S_INVALID:
		_state = S_ROM_CMD;
		clr_xfer_buf();
		break;

	case S_ROM_CMD:
		if (_xferBuf == 0xCC)
		{
			_state = S_CMD;
			clr_xfer_buf();
		}
		else if (_xferBuf == 0x33)
		{
			_state = S_ROM_READ;
			_addr = 0;
			set_xfer_buf(_data[_addr ++]);
		}
		else
		{
			_state = S_INVALID;
		}
		break;

	case S_ROM_READ:
		if (_addr < ROM_SIZE)
		{
			set_xfer_buf(_data[_addr ++]);
		}
		else
		{
			_state = S_INVALID;
		}
		break;

	case S_CMD:
		if (_xferBuf == 0xF0)
		{
			_crc = CRC_8(0, _xferBuf);
			_state = S_ADDR_LB;
			clr_xfer_buf();
		}
		else
		{
			_state = S_INVALID;
		}
		break;

	case S_ADDR_LB:
		// low byte of _address should be less than 0x80
		_xferBuf &= 0x7F;
		_addr = _xferBuf;
		_crc = CRC_8(_crc, _xferBuf);

		_state = S_ADDR_HB;
		clr_xfer_buf();
		break;

	case S_ADDR_HB:
		// high byte of _address should always be zero
		_crc = CRC_8(_crc, 0);

		_state = S_ADDR_CRC;
		set_xfer_buf(_crc);
		break;

	case S_ADDR_CRC:
		_addr += ROM_SIZE;
		_crc = CRC_8(0, _data[_addr]);

		_state = S_MEM_READ;
		set_xfer_buf(_data[_addr ++]);
		break;

	case S_MEM_READ:
		if (_addr < DATA_SIZE)
		{
			_crc = CRC_8(_crc, _data[_addr]);
			set_xfer_buf(_data[_addr ++]);
		}
		else
		{
			_state = S_MEM_CRC;
			set_xfer_buf(_crc);
		}
		break;

	default:
		_state = S_INVALID;
		clr_xfer_buf();
		break;
	}
}

// TACCR1 is used as a delay function mostly for releasing or sampling the line
// Unlike sampling the line can happen only if we are in receiving mode, releasing
// the line can happen either we are sending a 0 or we are lowering the line for
// a reset response. But we don't really care which case it is since all the event
// handling is already done at the time we lowering the line
#pragma vector = TIMER0_A1_VECTOR
__interrupt
void CCR1_ISR(void)
{
	if (TAIV == 2)
	{
		// shift the transfer buffer
		_xferBuf >>= 1;

		if (is_line_high())
		{
			// If the pin is high, we can only be in receiving mode
			_xferBuf |= 0x8000;
		}
		else
		{
			// other release the pin
			release_line();
		}

		if (_xferCnt == 0)
		{
			// calling state machine event handler
			_xferBuf >>= 8;
			OnXferDone();
		}

		// disable CCR1
		TACCTL1 = 0;
	}
}

static volatile uint16_t _downTime = 0;

//This ISR is used to detect:
// 1. the falling edge of the OW line, which is the start of an activity
// 2. the rising edge of the OW line, only for reset pulse detection
// 3. user button
#pragma vector = PORT1_VECTOR
__interrupt
void PORT1_ISR(void)
{
	if (P1IES & SLAVE_LINE)
	{
		// the content of _xferBuf is designed so that if the lowest
		// is 1, it must be in sending mode so that we can easily detect
		// this and pull the line down as fast as we can
		if (_xferBuf & 0x01)
		{
			hold_line_low();
		}

		if (_state != S_INVALID)
		{
			// decrease the # of bits remaining
			_xferCnt --;

			// do the other part of work after 20 us
			set_ccr1_timer(20 * CPU_FREQ);
		}

		// last thing to do is remember when the line is pulling down so that
		// we can detect reset pulse
		_downTime = TAR;
	}
	else
	{
		// rising edge, only used to detect reset pulse, once the pulse is
		// detected, we will pull the line down again and release it in 240 us
		if (TAR - _downTime > 480 * CPU_FREQ)
		{
			DELAY_US(25);
			hold_line_low();

			// to trigger a transfer done event
			_state = S_INVALID;
			_xferCnt = 0;
			set_ccr1_timer(100 * CPU_FREQ);
		}
	}

	// clear the interrupt flag
	P1IFG &= ~SLAVE_LINE;

	// this is to make sure we capture the next edge change. we have to toggle
	// this flag since msp430 doesn't provide an "interrupt on both edges" mode. A
	// potential risk is missing one edge, which will cause the signal not working
	// or a fake reset pulse
	P1IES ^= SLAVE_LINE;
}

uint16_t Disptch_OneWire_Events()
{
	// configure pins, enabling falling edge interrupt
	P1REN &= ~SLAVE_LINE;
	P1DIR &= ~SLAVE_LINE;
	P1IES |= SLAVE_LINE;
	P1IE |= SLAVE_LINE;

	// enabling timer block
	TACTL = TASSEL_2 | MC_2 | ID_0 | TACLR;

	// initial state is S_INVALID
	_state = S_INVALID;

	// clear transfer buffer
	_xferBuf = 0x00;

	// enabling interrupt
	__enable_interrupt();

	LPM0;

	__disable_interrupt();

	return _userEvent;
}
