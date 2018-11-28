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

+------------------+------------------+
| **Brand**        | **Protocol**     |
+------------------+------------------+
| Alecto WX500     | alecto_wx500     |
+------------------+------------------+
| Auriol H13726    | auriol_h13726    |
+------------------+------------------+
| Hama EWS1500     | hama_ews1500     |
+------------------+------------------+
| Meteoscan W1XXX  | meteoscan_w1XX   |
+------------------+------------------+
| Balance RF-WS105 | balance_rf_ws105 |
+------------------+------------------+

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
         "temperature": 18.90,
         "humidity": 41.00,
         "windavg": 10.00,
         "winddir": 90.00,
         "windgust": 20.00,
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
| temperature      | -40 - 60        |
+------------------+-----------------+
| humidity         | 10 - 99         |
+------------------+-----------------+
| windavg          | 0 - 50          |
+------------------+-----------------+
| windavg          | 0 - 359         |
+------------------+-----------------+
| windgust         | 0 - 50          |
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

.. rubric:: Protocol

A good description on this protocol can be found in `this <http://wiki.pilight.org/auriol_protocol_v20.pdf>`_ PDF.
