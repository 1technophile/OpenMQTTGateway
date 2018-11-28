.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

TCM 218943
==========

.. versionadded:: 8.0

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

+-------------+----------------+
| **Brand**   | **Protocol**   |
+-------------+----------------+
| TCM 218943  | tcm            |
+-------------+----------------+

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "weather": {
         "protocol": [ "tcm" ],
         "id": [{
           "id": 104,
         }],
         "temperature": 22.70,
         "humidity": 50.00,
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
| id               | 0 - 255         |
+------------------+-----------------+
| temperature      | -39.9 - 59.9    |
+------------------+-----------------+
| humidity         | 0 - 99          |
+------------------+-----------------+
| battery          | 0 - 1           |
+------------------+-----------------+

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

The tcm protocol sends 74 pulses like

.. code-block:: guess

   194 1035 194 1994 194 1994 194 1035 194 1994 194 1035 194 1035 194 1035 194 1035 194 1035 194 1035 194 1035 194 1035 194 1035 194 1035 194 1035 194 1035 194 1035 194 1994 194 1994 194 1035 194 1035 194 1994 194 1035 194 1035 194 1035 194 1035 194 1035 194 1994 194 1994 194 1994 194 1035 194 1035 194 1035 194 1994 194 1994 194 8037

The next step is to transform this output into groups of 2 pulses.

.. code-block:: guess

   194 1035
   194 1994
   194 1994
   194 1035
   194 1994
   194 1035
   194 1035
   194 1035
   194 1035
   194 1035
   194 1035
   194 1035
   194 1035
   194 1035
   194 1035
   194 1035
   194 1035
   194 1035
   194 1994
   194 1994
   194 1035
   194 1035
   194 1994
   194 1035
   194 1035
   194 1035
   194 1035
   194 1035
   194 1994
   194 1994
   194 1994
   194 1035
   194 1035
   194 1035
   194 1994
   194 1994
   194 8037

If we now look at carefully at these groups you can distinguish two types of groups:

#. ``194 1035``
#. ``194 1994``

So the first group is defined by a low 2nd, the second group has a high 2nd pulse. So we take either of these two pulses to define a 0 or a 1. In this case we say a high 2nd pulse means a 1 and a low 2nd pulse means a 0. We then get the following output:

.. code-block:: guess

   011010000000000000110010000011100011

Each (group) of numbers has a specific meaning:

- ID: 1 till 8
- Battery: 9
- Button: 12
- Humidity: 17 till 24
- Temperature: 25 till 36

.. code-block:: guess

   ID : 01101000 | Bat : 0 | But : 0 | Hum : 00110010 | Temp : 000011100011

- The ID is defined as a binary number.
- The Battery indicates low battery with 1.
- The Button is 1, if the button on the sensor is pressed.
- The Temperature is a 12 bits wide 2's complement signed binary number represents the actual temperature value in 0.1 °C units. DEC(AAAAAAAAAAAA)/10
- The Humidity is defined as a binary number.

.. code-block:: guess

   ID : 104 | Bat : OK | But : not pressed | Hum : 50% | Temp : 22.7°C

This protocol was created for pilight with the help of this thread: http://forum.arduino.cc/index.php?topic=136836.0 (german)

.. rubric:: Comments

Please be aware that the ID of a sensor changes after you replace the batteries.
