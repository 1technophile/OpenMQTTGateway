.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Cleverwatts
===========

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
| Cleverwatts          | cleverwatts      |
+----------------------+------------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -i --id=id             control a device with this id
   -u --unit=unit         control a device with this unit
   -t --on                send an on signal
   -f --off               send an off signal
   -a --all               send an all signal

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "cleverwatts" ],
         "id": [{
           "id": 73404,
           "unit": 0
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
| id               | 1 - 1048575     |
+------------------+-----------------+
| unit             | 0 - 3           |
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

   270 810 810 270 810 270 270 810 810 270 810 270 810 270 270 810 810 270 270 810 810 270 270 810 810 270 270 810 270 810 810 270 270 810 270 810 270 810 270 810 810 270 810 270 810 270 270 810 270 9180

It has no ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine, and the protocol also has some bit checks to filter false positives. We don't use them for further processing. The next step is to transform this output into 12 groups of 4 pulses (and thereby dropping the ``footer`` pulses).

.. code-block:: console

   270 810
   810 270
   810 270
   270 810
   810 270
   810 270
   810 270
   270 810
   810 270
   270 810
   810 270
   270 810
   810 270
   270 810
   270 810
   810 270
   270 810
   270 810
   270 810
   270 810
   810 270
   810 270
   810 270
   270 810

If we now look at carefully at these groups you can distinguish three types of groups:

#. ``270 810``
#. ``810 270``

- The first group is defined by a low 1st and high 2nd pulse, hence we call it low.
- The second group has a high 1st and low 2nd pulse, hence we call it high.

We then get the following output:

.. code-block:: console

   0 1
   1 0
   1 0
   0 1
   1 0
   1 0
   1 0
   0 1
   1 0
   0 1
   1 0
   0 1
   1 0
   0 1
   0 1
   1 0
   0 1
   0 1
   0 1
   0 1
   1 0
   1 0
   1 0
   0 1

We then remove the first column of numbers and then put the remaining numbers in a row:

.. code-block:: console

   1001000101011011110001

Each (group) of numbers has a specific meaning and are defined in binary format:

- ID: 0 till 19
- State: 20
- Unit: 21 and 22
- All: 23

So this code represents:

- Unit: 595311
- ID: 0
- State: On
- All: Single