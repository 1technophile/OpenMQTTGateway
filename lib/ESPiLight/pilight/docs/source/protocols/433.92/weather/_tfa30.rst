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
| Dostmann TFA 30.3200  | tfa30          |
+-----------------------+----------------+
| TFA30                 | tfa30          |
+-----------------------+----------------+


.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "weather": {
         "protocol": [ "tfa30" ],
         "id": [{
           "id": 228
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
| id               | 0 - 255         |
+------------------+-----------------+
| temperature      | -39.9 - 59.9    |
+------------------+-----------------+
| humidity         | 10 - 99         |
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

.. rubric:: Protocol

*After extensive testing it was detected that the Dostman TFA 30.3200 shares most parameters with the tfa and conrad weather station sensors, however due to the differences detected those are described in this document.*

The protocol sends 88 pulses like:

.. code-block:: guess

   603 7630 601 1881 605 1897 610 3803 605 3811 651 3798 618 1884 600 1911 597 3806 599 1909 590 1930 595 1906 594 1905 591 1920 584 3804 593 3829 585 3808 600 1905 591 1937 602 3805 594 3809 599 3821 590 1903 595 1908 593 3826 586 1908 596 3837 597 3825 569 1918 594 3809 597 1911 594 1903 594 1909 591 3829 585 1926 592 1907 590 1972 531 1917 586 1911 607 1895 589 1952 556 3818 590 3811 594 7639

The next step is to transform this output into groups of 2 pulses.

.. code-block:: guess

   603 7630
   601 1881
   605 1897
   610 3803
   605 3811
   651 3798
   618 1884
   600 1911
   597 3806
   599 1909
   590 1930
   595 1906
   594 1905
   591 1920
   584 3804
   593 3829
   585 3808
   600 1905
   591 1937
   602 3805
   594 3809
   599 3821
   590 1903
   595 1908
   593 3826
   586 1908
   596 3837
   597 3825
   569 1918
   594 3809
   597 1911
   594 1903
   594 1909
   591 3829
   585 1926
   592 1907
   590 1972
   531 1917
   586 1911
   607 1895
   589 1952
   556 3818
   590 3811
   594 7639

Analysing the pulsetrain, we can distinguish three types of groups:

- The first group is defined by a short 2nd
- The second group has a long 2nd pulse.
- The third group has an extended long 2nd pulse, this 3rd group forms the header and the footer pulse group.

The duration of the pulses varies between manufacturers.

**Dostmann TFA 30.3200**

#. ``605 1905``
#. ``605 3811``
#. ``594 7639``

Length of pulsetrain is 88.

**TFA30 and conrad_weather**

#. ``498 1992``
#. ``498 4233``
#. ``498 8466``

Length of pulsetrain is 80 or 86.

It is currently unknown wether the ``tfa`` and the ``conrad weather pro`` are using identical protocols or not, due to discrepencies in the documentation. The current version of the tfa protocol module can handle both variants using the tfa30 protocol. The differentiation is analysing the header information and the length of the pulsetrain automatically.

We take either of the two other pulse groups to define a logical 0 or a 1. A long 2nd pulse means a 1 and a short 2nd pulse means a 0.

For the above pulsetrain we get the following binary output:

.. code-block:: console

      MSB--LSB       AAAABBBBCCCC AAAABBBB
   00 11100100 0 001 110011100101 10100010 0 0 000011

Each (group) of numbers has a specific meaning:

**Dostmann TFA 30.3200**

- unknown: 0 till 1
- ID: 2 till 9
- unknown: 10
- Channel: 11 till 13
- Temperature: 14 till 25
- Humidity: 26 till 33
- Tx Button: 34
- Battery low: 35
- unknown: 36 till 41

**tfa30 and conrad_weather**

- unknown: 0 till 1
- ID: 2 till 9
- Channel: 12 till 13
- Temperature: 14 till 25
- Humidity: 26 till 33
- Battery: 34 till 35
- unknown: 36 till 41

.. code-block:: console

   ID : 11100100 | Channel : 01 | Temp : 0101 1110 1100 | Hum : 00101010 | Bar : 00

- The ``ID`` is defined as a binary number
- The ``Temperature`` is defined as a binary number and represents the temperature in Fahrenheit, starting at -90°F). Assuming that the Temp binary value is AAAABBBBCCCC, the formula to calculate the temperature value in °C is

.. code-block:: console

  ( ((DEC(CCCC)*256)+(DEC(BBBB)*16)+(DEC(AAAA)))/10 -90 -32) * (5/9)

The ``Humidity`` is defined as a binary number. Assuming that the Hum binary value is AAAABBBB, the formula to calculate the humidity percentage is DEC(BBBB)*16 + DEC(AAAA)

.. code-block:: console

   ID : 228 | Temp : 16.44°C | Hum : 42%

