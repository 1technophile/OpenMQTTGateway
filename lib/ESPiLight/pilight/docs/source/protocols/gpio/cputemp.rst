.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

CPU Temperature
===============

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
| Raspberry Pi     | cpu_temp     |
+------------------+--------------+
| Hummingboard     | cpu_temp     |
+------------------+--------------+

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "weather": {
         "protocol": [ "cpu_temp" ],
         "id": [{
           "id": 1
         }],
         "temperature": 59.397,
         "poll-interval": 10
       }
     },
     "gui": {
       "temperature": {
         "name": "Temperature Sensor",
         "group": [ "Misc" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-------------------+
| **Option**       | **Value**         |
+------------------+-------------------+
| id               | 0 - 99999         |
+------------------+-------------------+
| temperature      | -40.000 -  85.000 |
+------------------+-------------------+
| poll-interval    | 1 - 60            |
+------------------+-------------------+

.. rubric:: Optional Settings

:underline:`Device Settings`

+--------------------+-------------+------------+-------------------------------------------------+
| **Setting**        | **Default** | **Format** | **Description**                                 |
+--------------------+-------------+------------+-------------------------------------------------+
| poll-interval      | 5           | seconds    | What should be the poll interval of the sensors |
+--------------------+-------------+------------+-------------------------------------------------+
| temperature-offset | 0           | number     | Correct temperature value                       |
+--------------------+-------------+------------+-------------------------------------------------+

:underline:`GUI Settings`

+----------------------+-------------+------------+-----------------------------------------------------------+
| **Setting**          | **Default** | **Format** | **Description**                                           |
+----------------------+-------------+------------+-----------------------------------------------------------+
| temperature-decimals | 1           | number     | How many decimals the GUIs should display for temperature |
+----------------------+-------------+------------+-----------------------------------------------------------+
| show-temperature     | 1           | 1 or 0     | Don't display the temperature value                       |
+----------------------+-------------+------------+-----------------------------------------------------------+

.. rubric:: Comments

On the Raspberry Pi:

It is possible you need to first load the sensor module:

.. code-block:: console

   sudo modprobe bcm2835_thermal

To make sure this is done every reboot, edit ``/etc/modules`` and add the following lines:

.. code-block:: console

   bcm2835_thermal