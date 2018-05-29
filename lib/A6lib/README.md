# A6lib

An ESP8266/Arduino library for communicating with the AI-Thinker A6 GSM module.
It will probably also work with other GSM modules that use the AT command set,
like the SIM900.

## Details

This library is an adaptation of
[this sample module](https://github.com/SensorsIot/A6-GSM-Module). It is still
very incomplete, but at least it works somewhat. Please feel free to issue pull
requests to improve it.

## Usage

If you have the A6 breakout board, connect the Tx and Rx pins to your ESP8266.
The example assumes the TX pin is connected to D5 and the RX to D6. A6lib can
power-cycle the module if you connect a MOSFET to a pin and control the A6's
power supply with it.

For a sample circuit that uses this library, have a look at [the A6/ESP8266
breakout board](https://gitlab.com/stavros/A6-ESP8266-breakout/) I designed.

The A6's PWR pin should be permanently connected to Vcc (if you think that's
wrong or know a better way, please open an issue).

The code looks something like this:

~~~
// Instantiate the class with Tx, Rx (remember to swap them when connecting to
the A6, i.e. connect the A6's Rx pin to D6).
A6lib A6c(D6, D5);

// Initialize the modem, rebooting it if it fails to become ready.
do {
    // Power-cycle the module to reset it.
    A6c.powerCycle(D0);
} while (A6c.blockUntilReady(9600) != A6_OK);

// Start and place a call.
A6c.dial("1234567890");
delay(8000);

A6c.hangUp();
delay(8000);

A6c.redial();
delay(8000);

// Send a message.
A6c.sendSMS("+1234567890", "Hello there!");

// Get an SMS message from memory.
SMSmessage sms = A6c.readSMS(3);

// Delete an SMS message.
A6c.deleteSMS(3);

callInfo cinfo = A6c.checkCallStatus();
// This will be the calling number, "1234567890".
cinfo.number;
~~~

This library doesn't currently include any code to connect to the internet, but
a PR adding that would be very welcome.
