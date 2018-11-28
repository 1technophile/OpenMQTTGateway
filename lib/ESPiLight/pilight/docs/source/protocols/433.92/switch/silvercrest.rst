.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

SilverCrest
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
| Silvercrest          | silvercrest      |
+----------------------+------------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -t --on             send an on signal
   -f --off            send an off signal
   -u --unit=unit      control a device with this unit code
   -i --id=id          control a device with this id

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "silvercrest" ],
         "id": [{
           "systemcode": 22,
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

   312 936 936 312 312 936 312 936 312 936 312 936 312 936 936 312 312 936 312 936 312 936 936 312 312 936 312 936 312 936 936 312 312 936 936 312 312 936 936 312 312 936 936 312 312 936 312 936 312 10608

It has no ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine, and the protocol also has some bit checks to filter false positives. We don't use them for further processing. The next step is to transform this output into 12 groups of 4 pulses (and thereby dropping the ``footer`` pulses).

.. code-block:: console

   312 936 936 312
   312 936 312 936
   312 936 312 936
   312 936 936 312
   312 936 312 936

   312 936 936 312
   312 936 312 936
   312 936 936 312
   312 936 936 312
   312 936 936 312

   312 936 936 312
   312 936 312 936

   312 10608

If we now look at carefully at these groups you can distinguish three types of groups:

- ``312 936 936 312``
- ``312 936 312 936``

So the first group is defined by a high 3th pulse and the second group has a low 3rd pulse. In this case we say a high 3rd pulse means a 0 and a high 3rd pulse means a 1. We then get the following output:

.. code-block:: console

   01101 01000 01

Each (group) of numbers has a specific meaning:

- Unit: 0 till 5 (inversed)
- ID: 6 till 10 (inversed)
- Check: 11 (inverse of state)
- State: 12 (inversed)

.. code-block:: console

   10110 00010 0 0

- The ``Unit`` is defined as a binary number
- The ``ID`` is defined as a binary number
- The ``Check`` is always the inverse of the state
- The ``State`` defines whether a devices needs to be turned On or Off

So this code represents:

- Unit: 22
- ID: 2
- Fixed: Off
- State: On
