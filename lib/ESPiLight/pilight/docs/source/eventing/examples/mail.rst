.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Mail on contact trigger
=======================

.. rubric:: Description

Sending a mail based on a contact device trigger including the time when it happened.

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
     "frontdoor-contact": {
       "protocol": [ "kaku_contact" ],
       "id": [{
         "id": 123456,
         "unit": 0
       }],
       "state": "opened"
     }
   }

.. rubric:: Rule

.. code-block:: json
   :linenos:

   {
     "frontdoor-contact-mail": {
       "rule": "IF frontdoor-contact.state IS opened THEN sendmail TO foo@bar.com SUBJECT Frontdoor was opened at DATE_FORMAT(currentdatetime, %c) MESSAGE .",
       "active": 1
     }
   }

.. versionchanged:: 8.1.0

.. code-block:: json
   :linenos:

   {
     "frontdoor-contact-mail": {
       "rule": "IF frontdoor-contact.state == opened THEN sendmail TO foo@bar.com SUBJECT 'Frontdoor was opened at ' . DATE_FORMAT(currentdatetime, %c) MESSAGE ..",
       "active": 1
     }
   }