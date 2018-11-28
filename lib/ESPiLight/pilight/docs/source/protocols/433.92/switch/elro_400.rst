.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Elro HE (300 Series)
====================

+------------------+-------------+
| **Feature**      | **Support** |
+------------------+-------------+
| Sending          | |yes|       |
+------------------+-------------+
| Receiving        | |yes|       |
+------------------+-------------+
| Config           | |yes|       |
+------------------+-------------+

.. rubric:: Supported Brands

+----------------------+------------------+
| **Brand**            | **Protocol**     |
+----------------------+------------------+
| Elro 400 Series      | elro_400_switch  |
+----------------------+------------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -s --systemcode=systemcode     control a device with this systemcode
   -u --unitcode=unitcode         control a device with this unitcode
   -t --on                        send an on signal
   -f --off                       send an off signal

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "elro_400_switch" ],
         "id": [{
           "systemcode": 31,
           "unitcode": 0
         }],
         "state": "off"
       }
     },
     "gui": {
       "Lamp": {
         "name": "TV Backlit",
         "group": [ "Living" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| systemcode       | 0 - 31          |
+------------------+-----------------+
| unitcode         | 0 - 31          |
+------------------+-----------------+
| state            | on / off        |
+------------------+-----------------+

.. rubric:: Optional Settings

:underline:`GUI Settings`

+----------------------+-------------+------------+-----------------------------------------------------------+
| **Setting**          | **Default** | **Format** | **Description**                                           |
+----------------------+-------------+------------+-----------------------------------------------------------+
| readonly             | 1           | 1 or 0     | Disable controlling this device from the GUIs             |
+----------------------+-------------+------------+-----------------------------------------------------------+
| confirm              | 1           | 1 or 0     | Ask for confirmation when switching device                |
+----------------------+-------------+------------+-----------------------------------------------------------+

.. rubric:: Protocol

This protocol sends 50 pulses like this

.. code-block:: console

   320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 960 320 320 960 960 320 320 960 960 320 320 960 960 320 320 960 320 960 320 960 960 320 320 9920

It has no ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine, and the protocol also has some bit checks to filter false positives. We don't use them for further processing. The next step is to transform this output into 12 groups of 4 pulses (and thereby dropping the ``footer`` pulses).

.. code-block:: console

   320 960 320 960
   320 960 320 960
   320 960 320 960
   320 960 320 960
   320 960 320 960
   320 960 320 960
   320 960 960 320
   320 960 960 320
   320 960 960 320
   320 960 960 320
   320 960 320 960
   320 960 960 320
   320 9920

If we now look at carefully at these groups you can distinguish two types of groups:

- ``320 960 320 960``
- ``320 960 960 320``

So the first group is defined by a high 4th pulse and the second group has a low 4th pulse. In this case we say a high 4th pulse means a 0 and a low 4th pulse means a 1. We then get the following output:

.. code-block::console

   00000 01111 0 1

Each (group) of numbers has a specific meaning:

- SystemCode: 0 till 5
- UnitCode: 6 till 10
- State: 11 (inverse state)
- Check: 12

.. code-block: console

   00000 01111 0 1

- The ``SystemCode`` is defined as a binary number
- The ``UnitCode`` is defined as a binary number
- The ``State`` defines whether a devices needs to be turned On or Off
- The ``Check`` defines whether a devices needs to be turned On or Off (but is inverse)

So this code represents:

- SystemCode: 31 (inversed)
- UnitCode: 1 (inversed)
- State: On (inverse state)
- Check: On

Another example:

- SystemCode: 0 (inversed)
- UnitCode: 4 (inversed)
- State: Off (inverse state)
- Check: Off

.. code-block:: console

   11111 11011 1 0