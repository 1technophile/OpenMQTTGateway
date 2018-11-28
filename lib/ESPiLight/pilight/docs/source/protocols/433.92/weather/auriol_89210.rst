.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Auriol 89210
============

+------------------+-------------+
| **Feature**      | **Support** |
+------------------+-------------+
| Sending          | |no|        |
+------------------+-------------+
| Receiving        | |yes|       |
+------------------+-------------+
| Config           | |yes|       |
+------------------+-------------+

.. rubric:: Supported Brands

+------------------+------------------+
| **Brand**        | **Protocol**     |
+------------------+------------------+
| Auriol           | auriol           |
+------------------+------------------+

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "weather": {
         "protocol": [ "auriol" ],
         "id": [{
           "id": 3
         }],
         "temperature": 18.90,
         "humidity": 41.00,
         "battery": 1
        }
     },
     "gui": {
       "weather": {
         "name": "Weather Station",
         "group": [ "Outside" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 9           |
+------------------+-----------------+
| temperature      | 0 - 50          |
+------------------+-----------------+
| humidity         | 20 - 99         |
+------------------+-----------------+
| battery          | 0 - 1           |
+------------------+-----------------+

.. rubric:: Optional Settings

:underline:`Device Settings`

+--------------------+-------------+------------+---------------------------+
| **Setting**        | **Default** | **Format** | **Description**           |
+--------------------+-------------+------------+---------------------------+
| humidity-offset    | 0           | number     | Correct humidity value    |
+--------------------+-------------+------------+---------------------------+
| temperature-offset | 0           | number     | Correct temperature value |
+--------------------+-------------+------------+---------------------------+

:underline:`GUI Settings`

+----------------------+-------------+------------+-----------------------------------------------------------+
| **Setting**          | **Default** | **Format** | **Description**                                           |
+----------------------+-------------+------------+-----------------------------------------------------------+
| temperature-decimals | 2           | number     | How many decimals the GUIs should display for temperature |
+----------------------+-------------+------------+-----------------------------------------------------------+
| humidity-decimals    | 2           | number     | How many decimals the GUIs should display for humidity    |
+----------------------+-------------+------------+-----------------------------------------------------------+
| show-temperature     | 1           | 1 or 0     | Don't display the temperature value                       |
+----------------------+-------------+------------+-----------------------------------------------------------+
| show-humidity        | 1           | 1 or 0     | Don't display the humidity value                          |
+----------------------+-------------+------------+-----------------------------------------------------------+
| show-battery         | 1           | 1 or 0     | Don't display the battery value                           |
+----------------------+-------------+------------+-----------------------------------------------------------+

.. rubric:: Comments

This weather station was available at the Lidl. Description for device could be: “Auriol IAN 89210 FUNK-WEATHER STATION/RADIO CONTROLLED WEATHER”, Hama EWS-750 (not confirmed) and IAN 55982 (neither confirmed). The sensor can be identified by the option to choose from 3 different channels.

Humidity and battery assumed to be supported by this protocol, but might not be. The battery function is not tested 100% and humidity seems to vary regularly. Please be aware that the ID of a sensor is actually the channel.

.. rubric:: Protocol

This protocol has similarities with the protocols alecto and threechan.

Each (group of 2) numbers have the following specific meaning:

  - Battery ID 0 untill 7 (assumed)
  - Battery: 8
  - TX mode: 9
  - Channel: 10 untill 11 (3 = 10, 2 = 01, 1 = 11)
  - Temperature: 12 untill 23
  - Humidity: 24 untill 31 (assumed, alternatively this might be a checksum)
  - Footer: 32
  - The Battery ID is not used
  - The Battery identifies the state of the battery
  - The TX mode defines whether the signal was sent automatically or manually
  - The ID is defined as a binary number and specifies which channel the sensor uses
  - The Temperature is defined as a binary number and represents the temperature
  - The Humidity is defined as a binary number and represents the humidity (unconfirmed)

.. rubric:: Checksum

We need someone who is able to decipher the checksum and write a c snippet for it.

This protocol was created for pilight with the help of pilight-debug.