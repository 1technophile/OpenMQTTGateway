.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Mail on computer disconnection
==============================

.. rubric:: Description

Send a mail when one of the computer goes offline.

.. rubric:: Devices

.. code-block:: json
   :linenos:

   {
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
     "nasPing": {
       "protocol": [ "arping" ],
       "id": [{
         "mac": "aa:bb:cc:dd:00:11"
       }],
       "ip": "192.168.1.1",
       "state": "connected"
     }
   }

.. rubric:: Rule

.. code-block:: json
   :linenos:

   {
     "nas-mail": {
       "rule": "IF nasPing.state IS disconnected THEN sendmail TO foo@bar.com SUBJECT NAS is offline since DATE_FORMAT(currentdatetime, %c) MESSAGE .",
       "active": 1
     }
   }

.. versionchanged:: 8.1.0

.. code-block:: json
   :linenos:

   {
     "nas-mail": {
       "rule": "IF nasPing.state IS disconnected THEN sendmail TO foo@bar.com SUBJECT 'NAS is offline since ' .  DATE_FORMAT(currentdatetime, %c) MESSAGE ..",
       "active": 1
     }
   }