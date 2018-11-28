Wiring
======

- `Introduction`_
- `ATTiny pre-filter`_
- `pilight USB nano`_

Introduction
------------

The wiring of the different pilight hardware solutions will be described here in the best possible way. Please refer to the Configuration – Hardware page to see how the hardware modules are configured on the software side.

ATTiny pre-filter
-----------------

.. only:: html

   .. figure:: ../images/attiny-wiring.svg
      :align: left

.. only: latex

   .. image::  ../images/attiny-wiring.pdf

The ATTiny pre-filter is meant to filter the noise from 433.92Mhz receivers. Using a filter is not strictly necessary. However, it greatly reduces the CPU usage of your Raspberry Pi (and similar devices) so you can use it for more applications than just pilight.

Wiring the filter can be done it two steps. A required one, and an optional one. All optional wirings are made purple in the drawing. These wires are only used for firmware upgrades, which only happen rarely.

The different pins on the Raspberry Pi are numbered from bottom left (Pin 1) and top left (Pin 2) to bottom right (Pin 39) and top right (Pin 40).

The pins on the receiver (large rectangle) are numbered from left (Pin 1) to right (Pin 8).

The pins on the ATTiny are numbered from top left (Pin 1) to bottom left (Pin 4) and from bottom right (Pin 5) to top right (Pin 8).

The pins on the sender (small rectangle) are numbered from left (Pin 1) to right (Pin 4).

With this information, the following wiring scheme can be used for wiring up the ATTiny pre-filter together with the sender and receiver.

+--------------+------------------+-+------------+------------------+-+------------+------------------+
| **Receiver** | **ATTiny**       | | **ATTiny** | **Raspberry Pi** | | **Sender** | **Raspberry Pi** |
+--------------+------------------+ +------------+------------------+-+------------+------------------+
| Pin 7 (Data) | Pin 3            | | Pin 1      | Pin 24 (CE0)     | | Pin 1      | Pin 2 (5V)       |
+--------------+------------------+ +------------+------------------+-+------------+------------------+
| **Receiver** | **Raspberry Pi** | | Pin 2      | Pin 12 (GPIO 1)  | | Pin 2      | Pin 11 (GPIO 0)  |
+--------------+------------------+ +------------+------------------+-+------------+------------------+
| Pin 5 (VDD)  | Pin 2 (5V)       | | Pin 4      | Pin 6 (GND)      | | Pin 3      | Pin 3 (GND)      |
+--------------+------------------+ +------------+------------------+-+------------+------------------+
| Pin 8 (GND)  | Pin 6 (GND)      | | Pin 5      | Pin 19 (MOSI)    | |                               |
+--------------+------------------+ +------------+------------------+-+------------+------------------+
|                                 | | Pin 6      | Pin 21 (MISO)    | |                               |
+--------------+------------------+ +------------+------------------+-+------------+------------------+
|                                 | | Pin 7      | Pin 23 (SCLK)    | |                               |
+---------------------------------+-+------------+------------------+-+------------+------------------+

pilight USB nano
----------------

.. only:: html

   .. figure:: ../images/usb-nano-wiring.svg
      :align: left

.. only: latex

   .. image::  ../images/usb-nano-wiring.pdf

The pilight USB nano allows users to use pilight on any computer with USB support. The wiring scheme of the sender and receiver with the Arduino Nano is shown in the drawing. The Arduino Nano should be connected to the final computer over USB.

The pins on the Arduino Nano are as follow:

+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
| VIN | GND | RST | 5V  | A7  | A6  | A5  | A4  | A3  | A1  | A2  | A0  | REF | 3V3 | D13 |     |
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+ USB +
| TX1 | RX0 | RST | GND | D2  | D3  | D4  | D5  | D6  | D7  | D8  | D9  | D10 | D11 | D12 |     |
+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+

The pins on the receiver are numbered from left (Pin 1) to right (Pin 8).

The pins on the sender are numbered from left (Pin 1) to right (Pin 4).

With all this information, the following wiring scheme can be used for wiring up the pilight USB nano together with the sender and receiver.

+--------------+----------+-+--------------+----------+
| **Receiver** | **Nano** | | **Sender**   | **Nano** |
+--------------+----------+ +--------------+----------+
| Pin 1 (VDD)  | 5V       | | Pin 1 (GND)  | GND      |
+--------------+----------+ +--------------+----------+
| Pin 7 (Data) | D2       | | Pin 2 (Data) | D5       |
+--------------+----------+ +--------------+----------+
| Pin 8 (GND)  | GND      | | Pin 3 (VDD)  | 5V       |
+--------------+----------+-+--------------+----------+

