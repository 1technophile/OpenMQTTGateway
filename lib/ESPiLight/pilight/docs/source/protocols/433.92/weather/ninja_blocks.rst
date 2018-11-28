.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Ninja Blocks Weather Sensor
===========================

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

+------------------+---------------------+
| **Brand**        | **Protocol**        |
+------------------+---------------------+
| Ninja Blocks     | ninjablocks_weather |
+------------------+---------------------+

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "weather": {
         "protocol": [ "ninjablocks_weather" ],
         "id": [{
           "id": 1,
           "unit": 1
         }],
         "temperature": 18.90,
         "humidity": 41.00
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
| temperature      | -50 - 205.99    |
+------------------+-----------------+
| humidity         | 0 - 99          |
+------------------+-----------------+

.. rubric:: Optional Settings

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

.. rubric:: Comments

This weather station was available at the Lidl. Description for device could be: “Auriol IAN 89210 FUNK-WEATHER STATION/RADIO CONTROLLED WEATHER”, Hama EWS-750 (not confirmed) and IAN 55982 (neither confirmed). The sensor can be identified by the option to choose from 3 different channels.

Humidity and battery assumed to be supported by this protocol, but might not be. The battery function is not tested 100% and humidity seems to vary regularly. Please be aware that the ID of a sensor is actually the channel.

.. rubric:: Protocol

The protocol sends 74 pulses like:

.. code-block:: guess

   1083 860 966 986 1970 1953 2083 1829 1927 1014 938 1978 1943 993 946 1003 953 1983 2056 877 934 1020 931 1070 884 2013 1893 1012 930 2008 923 1019 1995 1930 925 1026 978 985 1891 1052 907 1036 913 2031 901 1034 916 1067 1881 1988 971 1004 890 1060 754 72330

The pulse stream uses the “Bi-Phase Mark Code” (BMC) or Frequencey/Double Frequency (F2F) principle as defined in ISO/IEC7811.

The first 6 pulses are the SYNC header and the last pulse is the footer. These are meant to identify the pulses as genuine. We check for their presence, an error in the first pulse of the header is tolerated, if the second sync pulse after the Unit code and the parity bit is correct, the length of the footer pulse is used to identify the protocol and as end of data transmission.

The first step is to transform this pulse stream into 36 logical bits We can group the sequence of bits into the following groups A to H:

.. code-block:: guess

   AAAA BBBB CC DDD EEEEEEE FFFFFFFFFFFFFFF G H
   1100 0001 00 110 0111001 010011011011001 1 F

Each of the groups of bits (A to H) has a specific meaning:

+-----------+-----------+-----------------+------------+----------------------------------------+
| **Group** | **Bit #** | **Config name** | **Range**  | **Description**                        |
+-----------+-----------+-----------------+------------+----------------------------------------+
| A         | 0-3                         | 12         | SYNC Header                            |
+-----------+-----------+-----------------+------------+----------------------------------------+
| B         | 4-7       | unit            | 0 to 15    | Unit Code                              |
+-----------+-----------+-----------------+------------+----------------------------------------+
| C         | 8-9       | id              | 0 to 3     | Channel Code                           |
+-----------+-----------+-----------------+------------+----------------------------------------+
| D         | 10-12                       | 110        | Sync Data                              |
+-----------+-----------+-----------------+------------+----------------------------------------+
| E         | 13-19     | humidity        | 0 to 100   | Humidity in %                          |
+-----------+-----------+-----------------+------------+----------------------------------------+
| F         | 20-34     | temperature     | 0 to 32767 | Temperature -50°C to 205,99°C          |
+-----------+-----------+-----------------+------------+----------------------------------------+
| G         | 35                          | 0,1        | Even Parity Bit                        |
+-----------+-----------+-----------------+------------+----------------------------------------+
| H         | 36                                       | Footer or gap pulse followed by Footer |
+-----------+-----------+-----------------+------------+----------------------------------------+

.. code-block:: console

   {
     "code": {
       "id": 0,
       "unit": 1,
       "temperature": 2769,
       "humidity": 5700
     },
     "origin": "receiver",
     "protocol": "ninjablocks_weather",
     "uuid": "0000-00-00-b4-46a46b",
     "repeats": 1
   }