.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Sunriseset
==========

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
         "protocol": [ "sunriseset" ],
         "id": [{
           "longitude": 1.2345,
           "latitude": 50.607080
         }],
         "sunrise": 7.00,
         "sunset": 16.15,
         "sun": "set"
        }
     },
     "gui": {
       "sun": {
         "name": "Sunrise / Sunset",
         "group": [ "Outside" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+----------------------+
| **Option**       | **Value**            |
+------------------+----------------------+
| longitude        | -180.0000 - 180.0000 |
+------------------+----------------------+
| latitude         | -90.0000 - 90.0000   |
+------------------+----------------------+
| sunrise          | 00.00 - 23.59        |
+------------------+----------------------+
| sunset           | 00.00 - 23.59        |
+------------------+----------------------+
| sun              | rise / set           |
+------------------+----------------------+

.. rubric:: Optional Settings

:underline:`GUI Settings`

+----------------------+-------------+------------+----------------------------------------------------------------------+
| **Setting**          | **Default** | **Format** | **Description**                                                      |
+----------------------+-------------+------------+----------------------------------------------------------------------+
| sunriseset-decimals  | 2           | number     | How many decimals the GUIs should display for sunrise/sunset time    |
+----------------------+-------------+------------+----------------------------------------------------------------------+
| show-sunriseset      | 0           | 1 or 0     | Don't display the sunrise /sunset                                    |
+----------------------+-------------+------------+----------------------------------------------------------------------+

.. rubric:: Comment

The sunriseset will send three messages:

#. When the sunset time has been arrived.
#. When the sunrise time has been arrived.
#. After midnight so all values can be updated for a new day.

When using sunriseset in eventing, keep in mind of the following; The output of sunriseset.sunset and sunriseset.sunrise is just a number, so the time 17:00 equals 17.00. When using it in eventing make sure you get usable numbers. For example the time for sunriseset.sunset is 16:30

.. code-block:: guess
   :linenos:

   IF ((sunrisesetdev.sunset == (datetime.hour + (datetime.minute / 100))) AND datetime.second == 0) THEN .."

pilight will see this:

.. code-block:: guess
   :linenos:

   IF ((16.30 == (16 + (30 / 100))) AND 0 == 0) THEN ..."

The math pilight will use: 16.30 == 16.30

When you look at the result you will see that the datetime.minute 30 will give 0.30 in the used rule (in normal case divide minutes through 100)

Using the sunriseset.

Example

.. code-block:: guess
   :linenos:

   IF sunrisesetdev.sun IS set THEN ....
   IF sunrisesetdev.sun IS rise THEN ....

Please note that with the three messages mentioned above, the first rule fires two times: At sunset, and at midnight (sunrisesetdev.sun is either set or rise, and will be set at midnight). So, it's better to use

.. code-block:: guess
   :linenos:

   IF (DATE_FORMAT(CurrentDateTime, %H.%M) == sunrisesetdev.sunset AND CurrentDateTime.second == 0) THEN ...

with CurrentDateTime as a ``datetime`` device.

.. rubric:: Notes

ntpserver has been removed from the sunriseset id. Instead, ntp-servers are defined in the settings section of config.json in an array like this:

.. code-block:: json
   :linenos:

   {
      "ntp-servers": [ "0.nl.pool.ntp.org", "1.nl.pool.ntp.org", "..." ],
      "ntp-sync": 1
   }

Both the datetime and sunriseset protocol will use the ntp time to adjust their time when needed.
