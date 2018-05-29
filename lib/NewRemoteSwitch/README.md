NewRemoteSwitch library v1.2.0 (20140128) for Arduino 1.0
Made by Randy Simons http://randysimons.nl/

This library provides an easy class for Arduino, to send and receive signals
used by some common "new style" 433MHz remote control switches.

There are two styles of remote:
 - "old style", which uses switches or a dial to set the house code. Use the
	RemoteSwitch library instead.
 - "new style", which use a button on the receivers to "learn" a signal. Use
   this library.

License: GPLv3. See ./NewRemoteSwitch/license.txt

Latest source and wiki: https://bitbucket.org/fuzzillogic/433mhzforarduino


Installation of library:
 - Make sure Arduino is closed
 - Copy the directory NewRemoteSwitch to the Arduino library directory (usually
   <Sketchbook directory>/libraries/)
   See http://arduino.cc/en/Guide/Libraries for detailed instructions.
 
Default installation demo:
 - Connect the data-out-pin of a 433MHz receiver to digital pin 2. See photo.
  (Note: your hardware may have a different pin configuration!)
 - Start Arduino, and open the example: File -> Examples -> NewRemoteSwitch ->
   ShowReceivedCode or ShowReceivedCodeNewRemote.
 - Compile, upload and run
 - Open serial monitor in Arduino (115200 baud)
 - Press buttons on a 433MHz-remote, and watch the serial monitor
 

Changelog:
NewRemoteSwitch library v1.2.0 (20140128) for Arduino 1.0
 - Revisited receiving dim-levels. Now it is always optional, check
   dimLevelPresent to see if it was transmitted.
   This change might break your code, as SwitchType::on_with_dim is removed.
 - Added NewRemoteTransmitter::sendGroupDim.
 - Added Dimmer example. Thanks to a Bitcoin donation, I could actually test it!

NewRemoteSwitch library v1.1.0 (20130601) for Arduino 1.0
 - BUGFIX: in many occasions, when receiving a dim-level, the code was rejected
   even if the signal was correct.
 - Support decoding A-series transmitters which transmit a dim-level in
   combination with an on-signal, instead of a dim-signal.
 - Uses NewRemodeCode::on, ::off, ::dim, ::on_with_dim instead of 0, 1, 2 and 3,
   for better readability. This change is backwards compatible.
 - Updated examples to use the new NewRemodeCode::on, ::off, ::dim and
   ::on_with_dim notation.
 - Reduced memory usage (Flash, RAM)

NewRemoteSwitch library v1.0.0 (20121229) for Arduino 1.0
 - Support for receiving A-series Klik-aan-klik-uit remote. (NewRemoteReceiver)
 - With examples to test and demonstrate.
