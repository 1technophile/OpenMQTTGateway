.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Weather
=======

+------------------+-------------+
| **Feature**      | **Support** |
+------------------+-------------+
| Sending          | |yes|       |
+------------------+-------------+
| Receiving        | |no|        |
+------------------+-------------+
| Config           | |yes|       |
+------------------+-------------+

.. rubric:: Supported Brands

*None*

.. rubric:: Sender Arguments

.. code-block:: console

   -t --temperature=temperature   set the temperature
   -h --humidity=humidity         set the humidity
   -b --battery=battery           set the battery level
   -i --id=id                     control a device with this id

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "weather": {
         "protocol": [ "generic_weather" ],
         "id": [{
           "id": 100
         }],
         "temperature": 23.00,
         "humidity": 76.00,
         "battery": 0
       }
     },
     "gui": {
       "weather": {
         "name": "Weather",
         "group": [ "Weather" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 99999       |
+------------------+-----------------+
| temperature      | 0.00 - 100.00   |
+------------------+-----------------+
| humidity         | 0.00 - 100.00   |
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
| show-humidity        | 1           | 1 or 0     | Don't display the humidity value                          |
+----------------------+-------------+------------+-----------------------------------------------------------+
| show-temperature     | 1           | 1 or 0     | Don't display the temperature value                       |
+----------------------+-------------+------------+-----------------------------------------------------------+
| show-battery         | 0           | 1 or 0     | Don't display the battery value                           |
+----------------------+-------------+------------+-----------------------------------------------------------+