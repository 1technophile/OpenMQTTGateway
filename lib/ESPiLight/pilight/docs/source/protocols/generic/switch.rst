.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Switch
======

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

   -t --on                  send an on signal
   -f --off                 send an off signal
   -i --id=id               control a device with this id

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "switch": {
         "protocol": [ "generic_switch" ],
         "id": [{
           "id": 100
         }],
         "state": "off"
       }
     },
     "gui": {
       "switch": {
         "name": "Switch",
         "group": [ "Living" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 99999       |
+------------------+-----------------+
| state            | on / off        |
+------------------+-----------------+

.. rubric:: Optional Settings

:underline:`GUI Settings`

+------------------+-------------+------------+-----------------------------------------------+
| **Setting**      | **Default** | **Format** | **Description**                               |
+------------------+-------------+------------+-----------------------------------------------+
| readonly         | 0           | 1 or 0     | Disable controlling this device from the GUIs |
+------------------+-------------+------------+-----------------------------------------------+
| confirm          | 0           | 1 or 0     | Ask for confirmation when switching device    |
+------------------+-------------+------------+-----------------------------------------------+
