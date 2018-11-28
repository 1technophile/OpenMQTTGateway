.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

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

+------------------+--------------+
| **Brand**        | **Protocol** |
+------------------+--------------+
| Alecto WSD17     | alecto_wsd17 |
+------------------+--------------+

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "weather": {
         "protocol": [ "alecto_wsd17" ],
         "id": [{
           "id": 100
         }],
         "temperature": 23.00
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
| temperature      | -50 - 70        |
+------------------+-----------------+

.. rubric:: Optional Settings

:underline:`Device Settings`

+--------------------+-------------+------------+---------------------------+
| **Setting**        | **Default** | **Format** | **Description**           |
+--------------------+-------------+------------+---------------------------+
| temperature-offset | 0           | number     | Correct temperature value |
+--------------------+-------------+------------+---------------------------+

:underline:`GUI Settings`

+----------------------+-------------+------------+-----------------------------------------------------------+
| **Setting**          | **Default** | **Format** | **Description**                                           |
+----------------------+-------------+------------+-----------------------------------------------------------+
| temperature-decimals | 2           | number     | How many decimals the GUIs should display for temperature |
+----------------------+-------------+------------+-----------------------------------------------------------+
| show-temperature     | 1           | 1 or 0     | Don't display the temperature value                       |
+----------------------+-------------+------------+-----------------------------------------------------------+
| show-humidity        | 1           | 1 or 0     | Don't display the humidity value                          |
+----------------------+-------------+------------+-----------------------------------------------------------+
| show-battery         | 1           | 1 or 0     | Don't display the battery value                           |
+----------------------+-------------+------------+-----------------------------------------------------------+

.. rubric:: Comment

This weather station or just the sensor is available from several (mostly Chinese) suppliers on eBay or Alibaba. Description for device is usually like: “Digital Wireless Indoor/Outdoor Weather Station with 3 Remote Sensors”. The sensor can be identified by the option to choose from 3 different channels.

Please be aware that the ID of a sensor changes after you replace the batteries.

.. rubric:: Protocol

The protocol sends 74 pulses like:

.. code-block:: guess

   540 1890 540 3780 540 1890 540 3780 540 3780 540 3780 540 3780 540 3780 540 1890 540 1890 540 3780 540 3780 540 3780 540 1890 540 1890 540 1890 540 1890 540 1890 540 1890 540 1890 540 3780 540 3780 540 1890 540 1890 540 4050 540 1890 540 4050 540 4050 540 1890 540 1890 540 4050 540 1890 540 3780 540 1890 540 3780 540 3780 540 9180

There first 8 pulses are the header and the last 2 pulses are the footer. These are meant to identify the pulses as genuine. We don't it for further processing. The next step is to transform this output into 36 groups of 2 pulses (and thereby dropping the footer pulses).

.. code-block:: guess

   540 1890
   540 3780
   540 1890
   540 3780
   540 3780
   540 3780
   540 3780
   540 3780
   540 1890
   540 1890
   540 3780
   540 3780
   540 3780
   540 1890
   540 1890
   540 1890
   540 1890
   540 1890
   540 1890
   540 1890
   540 3780
   540 3780
   540 1890
   540 1890
   540 4050
   540 1890
   540 4050
   540 4050
   540 1890
   540 1890
   540 4050
   540 1890
   540 3780
   540 1890
   540 3780
   540 3780

If we now look at carefully at these groups you can distinguish two types of groups:

#. ``540 1890``
#. ``540 3780``

So the first group is defined by a low 2nd, the second group has a high 2nd pulse. So we take either of these two pulses to define a 0 or a 1. In this case we say a high 2nd pulse means a 1 and a low 2nd pulse means a 0. We then get the following output:

.. code-block:: guess

   010111110011000000011001011001010111

Each (group) of numbers has a specific meaning:

- Header 0 till 3
- ID: 4 till 11
- Battery: 12
- TX mode: 13
- Channel: 14 till 15
- Temperature: 16 till 27
- Humidity: 28 till 35

.. code-block:: guess

   0101 11110011 0 0 00 000110010110 01010111

- The ID is defined as a binary number
- The Battery identifies the state of the battery
- The TX mode defines whether the signal was sent automatic or manual
- The Channel is defined as a binary number and specifies which channel the sensor uses
- The Temperature is defined as a binary number and represents the temperature
- The Humidity is defined as a binary number and represents the humidity

This protocol was created for pilight with the help of this thread: http://forum.pilight.org/Thread-Fully-Supported-No-brand-temp-humidity-sensor