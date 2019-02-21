# LoRa API

## Include Library

```arduino
#include <LoRa.h>
```

## Setup

### Begin

Initialize the library with the specified frequency.

```arduino
LoRa.begin(frequency);
```
 * `frequency` - frequency in Hz (`433E6`, `866E6`, `915E6`)

Returns `1` on success, `0` on failure.

### Set pins

Override the default `NSS`, `NRESET`, and `DIO0` pins used by the library. **Must** be called before `LoRa.begin()`.

```arduino
LoRa.setPins(ss, reset, dio0);
```
 * `ss` - new slave select pin to use, defaults to `10`
 * `reset` - new reset pin to use, defaults to `9`
 * `dio0` - new DIO0 pin to use, defaults to `2`.  **Must** be interrupt capable via [attachInterrupt(...)](https://www.arduino.cc/en/Reference/AttachInterrupt).

This call is optional and only needs to be used if you need to change the default pins used.

#### No MCU controlled reset pin

To save further pins one could connect the reset pin of the MCU with reset pin of the radio thus resetting only during startup.

* `reset` - set to `-1` to omit this pin

### Set SPI interface

Override the default SPI interface used by the library. **Must** be called before `LoRa.begin()`.

```arduino
LoRa.setSPI(spi);
```
 * `spi` - new SPI interface to use, defaults to `SPI`

This call is optional and only needs to be used if you need to change the default SPI interface used, in the case your Arduino (or compatible) board has more than one SPI interface present.

### Set SPI Frequency

Override the default SPI frequency of 10 MHz used by the library. **Must** be called before `LoRa.begin()`.

```arduino
LoRa.setSPIFrequency(frequency);
```
 * `frequency` - new SPI frequency to use, defaults to `8E6`

This call is optional and only needs to be used if you need to change the default SPI frequency used. Some logic level converters cannot support high speeds such as 8 MHz, so a lower SPI frequency can be selected with `LoRa.setSPIFrequency(frequency)`.

### End

Stop the library

```arduino
LoRa.end()
```

## Sending data

### Begin packet

Start the sequence of sending a packet.

```arduino
LoRa.beginPacket();

LoRa.beginPacket(implicitHeader);
```

 * `implicitHeader` - (optional) `true` enables implicit header mode, `false` enables explicit header mode (default)

Returns `1` on success, `0` on failure.

### Writing

Write data to the packet. Each packet can contain up to 255 bytes.

```arduino
LoRa.write(byte);

LoRa.write(buffer, length);
```
* `byte` - single byte to write to packet

or

* `buffer` - data to write to packet
* `length` - size of data to write

Returns the number of bytes written.

**Note:** Other Arduino `Print` API's can also be used to write data into the packet

### End packet

End the sequence of sending a packet.

```arduino
LoRa.endPacket()
```

Returns `1` on success, `0` on failure.

## Receiving data

### Parsing packet

Check if a packet has been received.

```arduino
int packetSize = LoRa.parsePacket();

int packetSize = LoRa.parsePacket(size);
```

 * `size` - (optional) if `> 0` implicit header mode is enabled with the expected a packet of `size` bytes, default mode is explicit header mode


Returns the packet size in bytes or `0` if no packet was received.

### Continuous receive mode

**WARNING**: Not supported on the Arduino MKR WAN 1300 board!

#### Register callback

Register a callback function for when a packet is received.

```arduino
LoRa.onReceive(onReceive);

void onReceive(int packetSize) {
 // ...
}
```

 * `onReceive` - function to call when a packet is received.

#### Receive mode

Puts the radio in continuous receive mode.

```arduino
LoRa.receive();

LoRa.receive(int size);
```

 * `size` - (optional) if `> 0` implicit header mode is enabled with the expected a packet of `size` bytes, default mode is explicit header mode

The `onReceive` callback will be called when a packet is received.

### Packet RSSI

```arduino
int rssi = LoRa.packetRssi();
```

Returns the RSSI of the received packet.

### Packet SNR

```arduino
float snr = LoRa.packetSnr();
```

Returns the estimated SNR of the received packet in dB.

### Packet Frequency Error

```arduino
long freqErr = LoRa.packetFrequencyError();
```

Returns the frequency error of the received packet in Hz. The frequency error is the frequency offset between the receiver centre frequency and that of an incoming LoRa signal.

### Available

```arduino
int availableBytes = LoRa.available()
```

Returns number of bytes available for reading.

### Peeking

Peek at the next byte in the packet.

```arduino
byte b = LoRa.peek();
```

Returns the next byte in the packet or `-1` if no bytes are available.

### Reading

Read the next byte from the packet.

```arduino
byte b = LoRa.read();
```

Returns the next byte in the packet or `-1` if no bytes are available.

**Note:** Other Arduino [`Stream` API's](https://www.arduino.cc/en/Reference/Stream) can also be used to read data from the packet

## Other radio modes

### Idle mode

Put the radio in idle (standby) mode.

```arduino
LoRa.idle();
```

### Sleep mode

Put the radio in sleep mode.

```arduino
LoRa.sleep();
```

## Radio parameters

### TX Power

Change the TX power of the radio.

```arduino
LoRa.setTxPower(txPower);

LoRa.setTxPower(txPower, outputPin);
```
 * `txPower` - TX power in dB, defaults to `17`
 * `outputPin` - (optional) PA output pin, supported values are `PA_OUTPUT_RFO_PIN` and `PA_OUTPUT_PA_BOOST_PIN`, defaults to `PA_OUTPUT_PA_BOOST_PIN`.

Supported values are between `2` and `17` for `PA_OUTPUT_PA_BOOST_PIN`, `0` and `14` for `PA_OUTPUT_RFO_PIN`.

Most modules have the PA output pin connected to PA BOOST,

### Frequency

Change the frequency of the radio.

```arduino
LoRa.setFrequency(frequency);
```
 * `frequency` - frequency in Hz (`433E6`, `866E6`, `915E6`)

### Spreading Factor

Change the spreading factor of the radio.

```arduino
LoRa.setSpreadingFactor(spreadingFactor);
```
 * `spreadingFactor` - spreading factor, defaults to `7`

Supported values are between `6` and `12`. If a spreading factor of `6` is set, implicit header mode must be used to transmit and receive packets.

### Signal Bandwidth

Change the signal bandwidth of the radio.

```arduino
LoRa.setSignalBandwidth(signalBandwidth);
```

 * `signalBandwidth` - signal bandwidth in Hz, defaults to `125E3`.

Supported values are `7.8E3`, `10.4E3`, `15.6E3`, `20.8E3`, `31.25E3`, `41.7E3`, `62.5E3`, `125E3`, and `250E3`.

### Coding Rate

Change the coding rate of the radio.

```arduino
LoRa.setCodingRate4(codingRateDenominator);
```

 * `codingRateDenominator` - denominator of the coding rate, defaults to `5`

Supported values are between `5` and `8`, these correspond to coding rates of `4/5` and `4/8`. The coding rate numerator is fixed at `4`.

### Preamble Length

Change the preamble length of the radio.

```arduino
LoRa.setPreambleLength(preambleLength);
```

 * `preambleLength` - preamble length in symbols, defaults to `8`

Supported values are between `6` and `65535`.

### Sync Word

Change the sync word of the radio.

```arduino
LoRa.setSyncWord(syncWord);
```

 * `syncWord` - byte value to use as the sync word, defaults to `0x34`

### CRC

Enable or disable CRC usage, by default a CRC is not used.

```arduino
LoRa.enableCrc();

LoRa.disableCrc();
```

## Other functions

### Random

Generate a random byte, based on the Wideband RSSI measurement.

```
byte b = LoRa.random();
```

Returns random byte.
