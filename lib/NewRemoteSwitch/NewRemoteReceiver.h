/*
 * NewRemoteSwitch library v1.2.0 (20140128) made by Randy Simons http://randysimons.nl/
 *
 * License: GPLv3. See license.txt
 */

#ifndef NewRemoteReceiver_h
#define NewRemoteReceiver_h

#include <Arduino.h>

typedef enum SwitchType {
   off = 0,
    on = 1,
    dim = 2
  } switchType;

struct NewRemoteCode {
	enum SwitchType {
   off = 0,
    on = 1,
    dim = 2
  };
	unsigned int period;		// Detected duration in microseconds of 1T in the received signal
	unsigned long address;		// Address of received code. [0..2^26-1]
	boolean groupBit;			// Group bit set or not
	SwitchType switchType;		// off, on, dim, on_with_dim.
	byte unit;					// Unit code of received code [0..15]
	boolean dimLevelPresent;	// Dim level present or not. Will be available for switchType dim, but might be available for on or off too, depending on remote.
	byte dimLevel;				// Dim level [0..15]. Will be available if switchType is dim, on_with_dim or off_with_dim.
};

#ifdef ESP8266
#include <functional>
#define CALLBACK_SIGNATUREH typedef std::function<void(unsigned int period, unsigned long address, unsigned long groupBit, unsigned long unit, unsigned long switchType)> NewRemoteReceiverCallBack
#else
#define CALLBACK_SIGNATUREH typedef void (*NewRemoteReceiverCallBack)(unsigned int period, unsigned long address, unsigned long groupBit, unsigned long unit, unsigned long switchType)
#endif
CALLBACK_SIGNATUREH;


/**
* See RemoteSwitch for introduction.
*
* NewRemoteReceiver decodes the signal received from a 433MHz-receiver, like the "KlikAanKlikUit"-system
* as well as the signal sent by the RemoteSwtich class. When a correct signal is received,
* a user-defined callback function is called.
*
* Note that in the callback function, the interrupts are still disabled. You can enabled them, if needed.
* A call to the callback must be finished before NewRemoteReceiver will call the callback function again, thus
* there is no re-entrant problem.
*
* When sending your own code using NewRemoteSwich, disable() the receiver first.
*
* This is a pure static class, for simplicity and to limit memory-use.
*/

		
class NewRemoteReceiver {
	public:
		/**
		* Initializes the decoder.
		*
		* If interrupt >= 0, init will register pin <interrupt> to this library.
		* If interrupt < 0, no interrupt is registered. In that case, you have to call interruptHandler()
		* yourself whenever the output of the receiver changes, or you can use InterruptChain.
		*
		* @param interrupt 	The interrupt as is used by Arduino's attachInterrupt function. See attachInterrupt for details.
							If < 0, you must call interruptHandler() yourself.
		* @param minRepeats The number of times the same code must be received in a row before the callback is calles
		* @param callback Pointer to a callback function, with signature void (*func)(NewRemoteCode)
		*/
		static void init(int8_t interrupt, byte minRepeats, NewRemoteReceiverCallBack callback);

		/**
		* Enable decoding. No need to call enable() after init().
		*/
		static void enable();

		/**
		* Disable decoding. You can re-enable decoding by calling enable();
		*/
		static void disable();

		/**
		* Deinitializes the decoder. Disables decoding and detaches the interrupt handler. If you want to
		* re-enable decoding, call init() again.
		*/
		static void deinit();

		/**
		* Tells wether a signal is being received. If a compatible signal is detected within the time out, isReceiving returns true.
		* Since it makes no sense to transmit while another transmitter is active, it's best to wait for isReceiving() to false.
		* By default it waits for 150ms, in which a (relative slow) KaKu signal can be broadcasted three times.
		*
		* Note: isReceiving() depends on interrupts enabled. Thus, when disabled()'ed, or when interrupts are disabled (as is
		* the case in the callback), isReceiving() will not work properly.
		*
		* @param waitMillis number of milliseconds to monitor for signal.
		* @return boolean If after waitMillis no signal was being processed, returns false. If before expiration a signal was being processed, returns true.
		*/
		static boolean isReceiving(int waitMillis = 150);

		/**
		 * Called every time the signal level changes (high to low or vice versa). Usually called by interrupt.
		 */
		static void interruptHandler();

	private:

		static int8_t _interrupt;					// Radio input interrupt
		volatile static short _state;				// State of decoding process.
		static byte _minRepeats;
		static NewRemoteReceiverCallBack _callback;
		static boolean _inCallback;					// When true, the callback function is being executed; prevents re-entrance.
		static boolean _enabled;					// If true, monitoring and decoding is enabled. If false, interruptHandler will return immediately.

};

#endif
