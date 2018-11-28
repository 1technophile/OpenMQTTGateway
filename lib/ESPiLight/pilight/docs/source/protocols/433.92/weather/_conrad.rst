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

+-----------------------+----------------+
| **Brand**             | **Protocol**   |
+-----------------------+----------------+
| Soens Weather Station | soens          |
+-----------------------+----------------+
| TFA                   | tfa            |
+-----------------------+----------------+
| Conrad                | conrad_weather |
+-----------------------+----------------+


.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "weather": {
         "protocol": [ "tfa" ],
         "id": [{
           "id": 108,
           "channel": 3
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
| temperature      | -39.9 - 59.9    |
+------------------+-----------------+
| humidity         | 10 - 99         |
+------------------+-----------------+
| battery          | 0 - 1           |
+------------------+-----------------+

.. note:: **Conrad**

   This sensor does not support humidity measurements

.. rubric:: Optional Settings

:underline:`Device Settings`

+--------------------+-------------+------------+---------------------------+
| **Setting**        | **Default** | **Format** | **Description**           |
+--------------------+-------------+------------+---------------------------+
| temperature-offset | 0           | number     | Correct temperature value |
+--------------------+-------------+------------+---------------------------+
| humidity-offset    | 0           | number     | Correct humidity value    |
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

.. rubric:: Protocol

The tfa protocol sends 86 pulses like

.. code-block:: guess

   498 1992 498 1992 498 1992 498 1992 498 1992 498 4233 498 4233 498 1992 498 1992 498 2241 498 1992 498 1992 498 2241 498 2241 498 1992 498 1992 498 4233 498 2241 498 2241 498 1992 498 1992 498 4482 498 1992 498 4233 498 4233 498 2241 498 4482 498 1992 498 1992 498 4233 498 1992 498 1992 498 4233 498 4233 498 1992 498 1992 498 1992 498 2241 498 2241 498 1992 498 4233 498 2241 498 8466

The next step is to transform this output into groups of 2 pulses.

.. code-block:: guess

   498 1992
   498 1992
   498 1992
   498 1992
   498 1992
   498 4233
   498 4233
   498 1992
   498 1992
   498 2241
   498 1992
   498 1992
   498 2241
   498 2241
   498 1992
   498 1992
   498 4233
   498 2241
   498 2241
   498 1992
   498 1992
   498 4482
   498 1992
   498 4233
   498 4233
   498 2241
   498 4482
   498 1992
   498 1992
   498 4233
   498 1992
   498 1992
   498 4233
   498 4233
   498 1992
   498 1992
   498 1992
   498 2241
   498 2241
   498 1992
   498 4233
   498 2241
   498 8466

If we now look at carefully at these groups you can distinguish two types of groups:

#. ``498 1992``
#. ``498 4233``

So the first group is defined by a low 2nd, the second group has a high 2nd pulse. So we take either of these two pulses to define a 0 or a 1. In this case we say a high 2nd pulse means a 1 and a low 2nd pulse means a 0. We then get the following output:

.. code-block:: guess

   000001100000000010000101101001001100000010

Each (group) of numbers has a specific meaning:

- ID: 2 till 9
- Channel: 12 till 13
- Temperature: 14 till 25
- Humidity: 26 till 33
- Battery: 34 till 35

.. code-block:: guess

   ID : 00011000 | Channel : 00 | Temp : 011000010010 | Hum : 00111001 | Bat : 00

- The ID is defined as a binary number
- The Channel is defined as a binary number and specifies which channel the sensor uses
- The Temperature is defined as a binary number and represents the temperature (value is given in fahrenheit strating at -90°F). Assuming that the Temp binary value is AAAABBBBCCCC, the formula to calculate the temperature value in °C is

.. code-block:: guess

   (((DEC(CCCC)*256) + (DEC(BBBB)*16) + (DEC(AAAA))/10) - 90 - 32) * (5/9)

- The ``Humidity`` is defined as a binary number. Assuming that the Hum binary value is AAAABBBB, the formula to calculate the humidity percentage is DEC(BBBB)*16 + DEC(AAAA)
- The ``Battery`` identifies the state of the battery

.. code-block:: guess

   ID : 24 | Channel : 1 | Temp : 18.56°C | Hum : 57% | Bat : OK

This protocol was created for pilight with the help of this thread: http://forum.pilight.org/Thread-New-Protocol-Wireless-Indoor-Outdoor-Weather-Station-with-3-Sensors

.. rubric:: Comments

This weather station or just the sensor is available from several (mostly Chinese) suppliers on eBay or Alibaba. Description for device is usually like: “Digital Wireless Indoor/Outdoor Weather Station with 3 Remote Sensors”. The sensor can be identified by the option to choose from 3 different channels.

Humidity and battery are supported by this protocol. The battery function is not tested 100%. Please be aware that the ID of a sensor changes after you replace the batteries. Conrad sensor only provide temperature (no humidity).
