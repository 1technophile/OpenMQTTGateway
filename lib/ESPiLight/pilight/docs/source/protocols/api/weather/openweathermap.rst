.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

OpenWeatherMap
==============

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

*None*

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "outside": {
         "protocol": [ "openweathermap" ],
         "id": [{
           "location": "amsterdam",
           "api": "1234567890",
           "country": "nl"
         }],
         "humidity": 76.00,
         "temperature": 7.00,
         "sunrise": 7.00,
         "sunset": 16.15,
         "sun": "set",
         "update": 1
        }
     },
     "gui": {
       "outside": {
         "name": "Temperature",
         "group": [ "Outside" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+----------------------------------+
| **Option**       | **Value**                        |
+------------------+----------------------------------+
| location         | *valid openweathermap location*  |
+------------------+----------------------------------+
| country          | *valid country abbreviation*     |
+------------------+----------------------------------+
| temperature      | -100.00 - 100.00                 |
+------------------+----------------------------------+
| humitidy         | 0.00 - 100.00                    |
+------------------+----------------------------------+
| sunrise          | 00.00 - 23.59                    |
+------------------+----------------------------------+
| sunset           | 00.00 - 23.59                    |
+------------------+----------------------------------+
| sun              | rise / set                       |
+------------------+----------------------------------+
| update           | 0 - 1                            |
+------------------+----------------------------------+

.. versionadded:: 8.0.7

+------------------+----------------------------------+
| **Option**       | **Value**                        |
+------------------+----------------------------------+
| api              | *valid openweathermap api key*   |
+------------------+----------------------------------+

.. rubric:: Optional Settings

:underline:`Device Settings`

+--------------------+-------------+------------+---------------------------------------------+
| **Setting**        | **Default** | **Format** | **Description**                             |
+--------------------+-------------+------------+---------------------------------------------+
| humidity-offset    | 0           | number     | Correct humidity value                      |
+--------------------+-------------+------------+---------------------------------------------+
| temperature-offset | 0           | number     | Correct temperature value                   |
+--------------------+-------------+------------+---------------------------------------------+
| poll-interval      | 86400       | seconds    | What should be the poll interval of the API |
+--------------------+-------------+------------+---------------------------------------------+

:underline:`GUI Settings`

+----------------------+-------------+------------+----------------------------------------------------------------------+
| **Setting**          | **Default** | **Format** | **Description**                                                      |
+----------------------+-------------+------------+----------------------------------------------------------------------+
| temperature-decimals | 2           | number     | How many decimals the GUIs should display for temperature            |
+----------------------+-------------+------------+----------------------------------------------------------------------+
| humidity-decimals    | 2           | number     | How many decimals the GUIs should display for humidity               |
+----------------------+-------------+------------+----------------------------------------------------------------------+
| sunriseset-decimals  | 2           | number     | How many decimals the GUIs should display for sunrise/sunset time    |
+----------------------+-------------+------------+----------------------------------------------------------------------+
| show-humidity        | 1           | 1 or 0     | Don't display the humidity value                                     |
+----------------------+-------------+------------+----------------------------------------------------------------------+
| show-temperature     | 1           | 1 or 0     | Don't display the temperature value                                  |
+----------------------+-------------+------------+----------------------------------------------------------------------+
| show-battery         | 0           | 1 or 0     | Don't display the battery value                                      |
+----------------------+-------------+------------+----------------------------------------------------------------------+


.. rubric:: Comment

#.  Please notice that the open weather map interval cannot be less than 10 minutes (600 seconds) to respect open weather map traffic and policy