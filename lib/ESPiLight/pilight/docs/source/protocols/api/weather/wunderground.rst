.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Weather Underground
===================

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
         "protocol": [ "wunderground" ],
         "id": [{
           "api": "1ihn472a6ea384jd",
           "location": "amsterdam",
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

+------------------+--------------------------------------+
| **Option**       | **Value**                            |
+------------------+--------------------------------------+
| api              | *valid weather underground api key*  |
+------------------+--------------------------------------+
| location         | *valid weather underground location* |
+------------------+--------------------------------------+
| country          | *valid country abbreviation*         |
+------------------+--------------------------------------+
| temperature      | -100.00 - 100.00                     |
+------------------+--------------------------------------+
| humitidy         | 0.00 - 100.00                        |
+------------------+--------------------------------------+
| sunrise          | 00.00 - 23.59                        |
+------------------+--------------------------------------+
| sunset           | 00.00 - 23.59                        |
+------------------+--------------------------------------+
| sun              | rise / set                           |
+------------------+--------------------------------------+
| update           | 0 - 1                                |
+------------------+--------------------------------------+

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


#. Please notice that the weather underground interval cannot be less than 15 minutes due to daily polling restrictions.
#. This restriction also counts for the update button in the webgui. You won't receive updates within the 15 minutes between two updates.
#. This wiki page shows a random API key. Please register yourself at wunderground.com to get your own.