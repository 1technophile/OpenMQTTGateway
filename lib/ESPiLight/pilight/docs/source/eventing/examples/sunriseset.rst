.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Sunrise / Sunset Event
======================

.. rubric:: Description

This rule executes when the sun sets or rises.

.. rubric:: Devices

.. code-block:: json
   :linenos:

   {
     "devices": {
       "currentdatetime": {
         "protocol": [ "datetime" ],
         "id": [{
           "longitude": 1.2345,
           "latitude": 12.3456
         }],
         "year": 2015,
         "month": 1,
         "day": 27,
         "hour": 14,
         "minute": 37,
         "second": 8,
         "weekday": 3,
         "dst": 1
       },
       "sunrisesetdevice": {
         "protocol": [ "sunriseset" ],
         "id": [{
           "longitude": 1.234567,
           "latitude": 12.123456
         }],
         "sunrise": 6.40,
         "sunset": 18.50,
         "sun": "rise"
       },
       "light": {
         "protocol": [ "kaku_switch" ],
         "id": [{
           "id": 123456,
           "unit": 0
         }],
         "state": "off"
       }
     }
   }

.. rubric:: Rule

By using math

.. code-block:: json
   :linenos:

   {
     "sunset": {
       "rule": "IF ((sunrisesetdevice.sunset == (currentdatetime.hour + (currentdatetime.minute / 100)) AND light.state IS off) AND currentdatetime.second == 0) THEN switch DEVICE light TO on",
       "active": 1
     },
     "sunrise": {
       "rule": "IF ((sunrisesetdevice.sunrise == (currentdatetime.hour + (currentdatetime.minute / 100)) AND light.state IS off) AND currentdatetime.second == 0) THEN switch DEVICE light TO on",
       "active": 1
     }
   }

By using the DATE_FORMAT function

.. code-block:: json
   :linenos:

   {
     "sunset": {
       "rule": "IF (sunrisesetdevice.sunset == DATE_FORMAT(currentdatetime, %H.%M)) AND light.state IS off AND currentdatetime.second == 0 THEN switch DEVICE light TO on",
       "active": 1
     },
     "sunrise": {
       "rule": "IF (sunrisesetdevice.sunrise == DATE_FORMAT(currentdatetime, %H.%M)) AND light.state IS off AND currentdatetime.second == 0 THEN switch DEVICE light TO on",
       "active": 1
     }
   }

Or an hour before the sun sets:

.. code-block:: json
   :linenos:

   {
     "sunset": {
       "rule": "IF (sunrisesetdevice.sunset == DATE_FORMAT(DATE_ADD(currentdatetime, +1 HOUR), \"%Y-%m-%d %H:%M:%S\", %H.%M)) AND light.state IS off AND currentdatetime.second == 0 THEN switch DEVICE light TO on",
       "active": 1
     }
   }

.. versionchanged:: 8.1.0

By using math

.. code-block:: json
   :linenos:

   {
     "sunset": {
       "rule": "IF ((sunrisesetdevice.sunset == (currentdatetime.hour + (currentdatetime.minute / 100)) AND light.state == off) AND currentdatetime.second == 0) THEN switch DEVICE light TO on",
       "active": 1
     },
     "sunrise": {
       "rule": "IF ((sunrisesetdevice.sunrise == (currentdatetime.hour + (currentdatetime.minute / 100)) AND light.state == off) AND currentdatetime.second == 0) THEN switch DEVICE light TO on",
       "active": 1
     }
   }

By using the DATE_FORMAT function

.. code-block:: json
   :linenos:

   {
     "sunset": {
       "rule": "IF (sunrisesetdevice.sunset == DATE_FORMAT(currentdatetime, %H.%M)) AND light.state == off AND currentdatetime.second == 0 THEN switch DEVICE light TO on",
       "active": 1
     },
     "sunrise": {
       "rule": "IF (sunrisesetdevice.sunrise == DATE_FORMAT(currentdatetime, %H.%M)) AND light.state == off AND currentdatetime.second == 0 THEN switch DEVICE light TO on",
       "active": 1
     }
   }

Or an hour before the sun sets:

.. code-block:: json
   :linenos:

   {
     "sunset": {
       "rule": "IF (sunrisesetdevice.sunset == DATE_FORMAT(DATE_ADD(currentdatetime, '+1 HOUR'), '%Y-%m-%d %H:%M:%S', %H.%M)) AND light.state == off AND currentdatetime.second == 0 THEN switch DEVICE light TO on",
       "active": 1
     }
   }