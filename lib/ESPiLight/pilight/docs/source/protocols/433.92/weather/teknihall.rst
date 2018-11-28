.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Teknihall GT-WT-02
==================

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

+--------------------+------------------+
| **Brand**          | **Protocol**     |
+--------------------+------------------+
| Teknihall GT-WT-02 | teknihall        |
+--------------------+------------------+

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "weather": {
         "protocol": [ "teknihall" ],
         "id": [{
           "id": 108
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
| id               | 0 - 3           |
+------------------+-----------------+
| unit             | 0 - 15          |
+------------------+-----------------+
| temperature      | -15 - 60        |
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
| show-wind            | 1           | 1 or 0     | Don't display the wind value                              |
+----------------------+-------------+------------+-----------------------------------------------------------+
| show-battery         | 1           | 1 or 0     | Don't display the battery value                           |
+----------------------+-------------+------------+-----------------------------------------------------------+

.. rubric:: Comments

This weather station is available at Aldi. Please be aware that there are several types of Teknihall weather stations and sensors available. Not all types might be compatible.

Please be aware that the ID of a sensor could change after you replace the batteries.

.. rubric:: Protocol

The protocol sends 76 pulses like:

.. code-block:: guess

   532 2128 532 4256 532 4256 532 2128 532 4256 532 4256 532 2128 532 2128 532 2128 532 2128 532 2128 532 2128 532 2128 532 2128 532 2128 532 2128 532 4256 532 2128 532 4256 532 4256 532 4256 532 4256 532 2128 532 2128 532 2128 532 4256 532 4256 532 4256 532 2128 532 2128 532 2128 532 4256 532 4256 532 2128 532 2128 532 2128 532 2128 532 9044

The last 2 pulses are the footer. These are meant to identify the pulses as genuine. We don't it for further processing. The next step is to transform this output into 38 groups of 2 pulses (and thereby dropping the footer pulses).

.. code-block:: guess

   532 2128
   532 4256
   532 4256
   532 2128
   532 4256
   532 4256
   532 2128
   532 2128
   532 2128
   532 2128
   532 2128
   532 2128
   532 2128
   532 2128
   532 2128
   532 2128
   532 4256
   532 2128
   532 4256
   532 4256
   532 4256
   532 4256
   532 2128
   532 2128
   532 2128
   532 4256
   532 4256
   532 4256
   532 2128
   532 2128
   532 2128
   532 4256
   532 4256
   532 2128
   532 2128
   532 2128
   532 2128
   532 9044

If we now look at carefully at these groups you can distinguish two types of groups:

- ``540 2128``
- ``540 4256``

So the first group is defined by a low 2nd, the second group has a high 2nd pulse. So we take either of these two pulses to define a 0 or a 1. In this case we say a high 2nd pulse means a 1 and a low 2nd pulse means a 0. The binary conversion could look as follows:

.. code-block:: guess

    0101111100111000000011001011001010111

Each (group) of numbers has a specific meaning:

- ID: 0 till 7
- Temperature: 14 till 23
- Humidity: 24 till 30

.. code-block:: guess

   01011111 001110 0000001100 1011001 010111


- The ID is defined as a binary number
- The Temperature is defined as a binary number and represents the temperature
- The Humidity is defined as a binary number and represents the humidity

This protocol was created for pilight with the help of this thread: http://forum.pilight.org/Thread-Connect-digital-weather-sensor