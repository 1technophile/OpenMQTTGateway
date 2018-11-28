.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Time based
==========

.. rubric:: Description

Trigger events based on time

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
     "outsidelight": {
       "protocol": [ "kaku_switch" ],
       "id": [{
         "id": 123456,
         "unit": 0
       }],
       "state": "off"
     }
   }

.. rubric:: Rule

.. code-block:: json
   :linenos:

   {
     "christmass-tree-off": {
       "rule": "IF DATE_FORMAT(currentdatetime, %H.%M%S) == 23.0000 THEN switch DEVICE outsidelight TO on AFTER RANDOM(0, 90) MINUTE FOR RANDOM(5, 15) MINUTE",
       "active": 1
     }
   }

.. versionchanged:: 8.1.0

.. code-block:: json
   :linenos:

   {
     "christmass-tree-off": {
       "rule": "IF DATE_FORMAT(currentdatetime, %H.%M%S) == 23.0000 THEN switch DEVICE outsidelight TO on AFTER RANDOM(0, 90) . ' MINUTE' FOR RANDOM(5, 15) . ' MINUTE'",
       "active": 1
     }
   }