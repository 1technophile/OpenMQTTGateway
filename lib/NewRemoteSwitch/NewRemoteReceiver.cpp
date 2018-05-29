/*
 * NewRemoteSwitch library v1.2.0 (20140128) made by Randy Simons http://randysimons.nl/
 * See NewRemoteReceiver.h for details.
 *
 * License: GPLv3. See license.txt
 */

#include "NewRemoteReceiver.h"

#define RESET_STATE _state = -1 // Resets state to initial position.

/************
* NewRemoteReceiver

Protocol. (Copied from Wieltje, http://www.circuitsonline.net/forum/view/message/1181410#1181410,
but with slightly different timings, as measured on my device.)
		_   _
'0':   | |_| |_____ (T,T,T,5T)
		_       _
'1':   | |_____| |_	(T,5T,T,T)
		_   _
dim:   | |_| |_     (T,T,T,T)

T = short period of ~260µs. However, this code tries
to figure out the correct period

A full frame looks like this:

- start pulse: 1T high, 10.44T low
- 26 bit:  Address
- 1  bit:  group bit
- 1  bit:  on/off/[dim]
- 4  bit:  unit
- [4 bit:  dim level. Present of [dim] is chosen, but might be present anyway...]
- stop pulse: 1T high, 40T low

************/

#ifdef ESP8266
    // interrupt handler and related code must be in RAM on ESP8266,
    #define RECEIVE_ATTR ICACHE_RAM_ATTR
	#define CALLBACK_SIGNATURE (_callback)(receivedCode.period, receivedCode.address, receivedCode.groupBit, receivedCode.unit, receivedCode.switchType)
#else
    #define RECEIVE_ATTR
	#define CALLBACK_SIGNATURE (_callback)(receivedCode.period, receivedCode.address, receivedCode.groupBit, receivedCode.unit, receivedCode.switchType)
#endif

int8_t NewRemoteReceiver::_interrupt;
volatile short NewRemoteReceiver::_state;
byte NewRemoteReceiver::_minRepeats;
NewRemoteReceiverCallBack NewRemoteReceiver::_callback;
boolean NewRemoteReceiver::_inCallback = false;
boolean NewRemoteReceiver::_enabled = false;

void NewRemoteReceiver::init(int8_t interrupt, byte minRepeats, NewRemoteReceiverCallBack callback) {
	_interrupt = interrupt;
	_minRepeats = minRepeats;
	_callback = callback;

	enable();
	if (_interrupt >= 0) {
		attachInterrupt(_interrupt, interruptHandler, CHANGE);
	}
}

void NewRemoteReceiver::enable() {
	RESET_STATE;
	_enabled = true;
}

void NewRemoteReceiver::disable() {
	_enabled = false;
}

void NewRemoteReceiver::deinit() {
	_enabled = false;
	if (_interrupt >= 0) {
		detachInterrupt(_interrupt);
	}
}

void RECEIVE_ATTR NewRemoteReceiver::interruptHandler() {
	// This method is written as compact code to keep it fast. While breaking up this method into more
	// methods would certainly increase the readability, it would also be much slower to execute.
	// Making calls to other methods is quite expensive on AVR. As These interrupt handlers are called
	// many times a second, calling other methods should be kept to a minimum.

	if (!_enabled) {
		return;
	}

	static byte receivedBit;		// Contains "bit" currently receiving
	static NewRemoteCode receivedCode;		// Contains received code
	static NewRemoteCode previousCode;		// Contains previous received code
	static byte repeats = 0;		// The number of times the an identical code is received in a row.
	static unsigned long edgeTimeStamp[3] = {0, };	// Timestamp of edges
	static unsigned int min1Period, max1Period, min5Period, max5Period;
	static bool skip;

	// Filter out too short pulses. This method works as a low pass filter.
	edgeTimeStamp[1] = edgeTimeStamp[2];
	edgeTimeStamp[2] = micros();

	if (skip) {
		skip = false;
		return;
	}

	if (_state >= 0 && edgeTimeStamp[2]-edgeTimeStamp[1] < min1Period) {
		// Last edge was too short.
		// Skip this edge, and the next too.
		skip = true;
		return;
	}

	unsigned int duration = edgeTimeStamp[1] - edgeTimeStamp[0];
	edgeTimeStamp[0] = edgeTimeStamp[1];

	// Note that if state>=0, duration is always >= 1 period.

	if (_state == -1) {
		// wait for the long low part of a stop bit.
		// Stopbit: 1T high, 40T low
		// By default 1T is 260µs, but for maximum compatibility go as low as 120µs
		if (duration > 4800) { // =40*120µs, minimal time between two edges before decoding starts.
			// Sync signal received.. Preparing for decoding
			repeats = 0;

			receivedCode.period = duration / 40; // Measured signal is 40T, so 1T (period) is measured signal / 40.

			// Allow for large error-margin. ElCheapo-hardware :(
			min1Period = receivedCode.period * 3 / 10; // Lower limit for 1 period is 0.3 times measured period; high signals can "linger" a bit sometimes, making low signals quite short.
			max1Period = receivedCode.period * 3; // Upper limit for 1 period is 3 times measured period
			min5Period = receivedCode.period * 3; // Lower limit for 5 periods is 3 times measured period
			max5Period = receivedCode.period * 8; // Upper limit for 5 periods is 8 times measured period
		}
		else {
			return;
		}
	} else if (_state == 0) { // Verify start bit part 1 of 2
		// Duration must be ~1T
		if (duration > max1Period) {
			RESET_STATE;
			return;
		}
		// Start-bit passed. Do some clean-up.
		receivedCode.address = receivedCode.unit = receivedCode.dimLevel = 0;
	} else if (_state == 1) { // Verify start bit part 2 of 2
		// Duration must be ~10.44T
		if (duration < 7 * receivedCode.period || duration > 15 * receivedCode.period) {
			RESET_STATE;
			return;
		}
	} else if (_state < 148) { // state 146 is first edge of stop-sequence. All bits before that adhere to default protocol, with exception of dim-bit
		receivedBit <<= 1;

		// One bit consists out of 4 bit parts.
		// bit part durations can ONLY be 1 or 5 periods.
		if (duration <= max1Period) {
			receivedBit &= B1110; // Clear LSB of receivedBit
		} else if (duration >= min5Period && duration <= max5Period) {
			receivedBit |= B1; // Set LSB of receivedBit
		} else if (
			// Check if duration matches the second part of stopbit (duration must be ~40T), and ...
			(duration >= 20 * receivedCode.period && duration <= 80 * receivedCode.period) &&
			// if first part op stopbit was a short signal (short signal yielded a 0 as second bit in receivedBit), and ...
			((receivedBit & B10) == B00) &&
			// we are in a state in which a stopbit is actually valid, only then ...
			(_state == 147 || _state == 131) ) {
				// If a dim-level was present...
				if (_state == 147) {
					// mark received switch signal as signal-with-dim
                    receivedCode.dimLevelPresent = true;
				}

				// a valid signal was found!
				if (
						receivedCode.address != previousCode.address ||
						receivedCode.unit != previousCode.unit ||
						receivedCode.dimLevelPresent != previousCode.dimLevelPresent ||
						receivedCode.dimLevel != previousCode.dimLevel ||
						receivedCode.groupBit != previousCode.groupBit ||
						receivedCode.switchType != previousCode.switchType
					) { // memcmp isn't deemed safe
					repeats=0;
					previousCode = receivedCode;
				}

				repeats++;

				if (repeats>=_minRepeats) {
					if (!_inCallback) {
						_inCallback = true;
						CALLBACK_SIGNATURE;
						_inCallback = false;
					}
					// Reset after callback.
					RESET_STATE;
					return;
				}

				// Reset for next round
				_state=0; // no need to wait for another sync-bit!
				return;
		}
		else { // Otherwise the entire sequence is invalid
			RESET_STATE;
			return;
		}

		if (_state % 4 == 1) { // Last bit part? Note: this is the short version of "if ( (_state-2) % 4 == 3 )"
			// There are 3 valid options for receivedBit:
			// 0, indicated by short short short long == B0001.
			// 1, short long shot short == B0100.
			// dim, short shot short shot == B0000.
			// Everything else: inconsistent data, trash the whole sequence.


			if (_state < 106) {
				// States 2 - 105 are address bit states

				receivedCode.address <<= 1;

				// Decode bit. Only 4 LSB's of receivedBit are used; trim the rest.
				switch (receivedBit & B1111) {
					case B0001: // Bit "0" received.
						// receivedCode.address |= 0; But let's not do that, as it is wasteful.
						break;
					case B0100: // Bit "1" received.
						receivedCode.address |= 1;
						break;
					default: // Bit was invalid. Abort.
						RESET_STATE;
						return;
				}
			} else if (_state < 110) {
				// States 106 - 109 are group bit states.
				switch (receivedBit & B1111) {
					case B0001: // Bit "0" received.
						receivedCode.groupBit = false;
						break;
					case B0100: // Bit "1" received.
						receivedCode.groupBit = true;
						break;
					default: // Bit was invalid. Abort.
						RESET_STATE;
						return;
				}
			} else if (_state < 114) {
				// States 110 - 113 are switch bit states.
				switch (receivedBit & B1111) {
					case B0001: // Bit "0" received.
						receivedCode.switchType = NewRemoteCode::off;
						break;
					case B0100: // Bit "1" received. Note: this might turn out to be a on_with_dim signal.
						receivedCode.switchType = NewRemoteCode::on;
						break;
					case B0000: // Bit "dim" received.
						receivedCode.switchType = NewRemoteCode::dim;
						break;
					default: // Bit was invalid. Abort.
						RESET_STATE;
						return;
				}
			} else if (_state < 130){
				// States 114 - 129 are unit bit states.
				receivedCode.unit <<= 1;

				// Decode bit.
				switch (receivedBit & B1111) {
					case B0001: // Bit "0" received.
						// receivedCode.unit |= 0; But let's not do that, as it is wasteful.
						break;
					case B0100: // Bit "1" received.
						receivedCode.unit |= 1;
						break;
					default: // Bit was invalid. Abort.
						RESET_STATE;
						return;
				}

			} else if (_state < 146) {
				// States 130 - 145 are dim bit states.
                // Depending on hardware, these bits can be present, even if switchType is NewRemoteCode::on or NewRemoteCode::off

				receivedCode.dimLevel <<= 1;

				// Decode bit.
				switch (receivedBit & B1111) {
					case B0001: // Bit "0" received.
						// receivedCode.dimLevel |= 0; But let's not do that, as it is wasteful.
						break;
					case B0100: // Bit "1" received.
						receivedCode.dimLevel |= 1;
						break;
					default: // Bit was invalid. Abort.
						RESET_STATE;
						return;
				}
			}
		}
	}

	_state++;
	return;
}

boolean NewRemoteReceiver::isReceiving(int waitMillis) {
	unsigned long startTime=millis();

	int waited; // Signed int!
	do {
		if (_state >= 34) { // Abort if a significant part of a code (start pulse + 8 bits) has been received
			return true;
		}
		waited = (millis()-startTime);
	} while(waited>=0 && waited <= waitMillis); // Yes, clock wraps every 50 days. And then you'd have to wait for a looooong time.

	return false;
}
