.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

SelectRemote
============

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
| SelectRemote         | selectremote     |
+----------------------+------------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -i --id=id                    control a device with this id
   -t --on                       send an on signal
   -f --off                      send an off signal

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "selectremote" ],
         "id": [{
           "id": 4
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
| id               | 0 - 7           |
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

   396 1188 396 1188 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 1188 396 13464

It has no ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine, and the protocol also has some bit checks to filter false positives. We don't use them for further processing. The next step is to transform this output into 12 groups of 4 pulses (and thereby dropping the ``footer`` pulses).

.. code-block:: console

   390 1170 1170 390
   390 1170 1170 390
   390 1170 1170 390
   390 1170 390 1170

   390 1170 390 1170
   390 1170 1170 390
   390 1170 1170 390
   390 1170 1170 390

   390 1170 1170 390
   390 1170 1170 390
   390 1170 1170 390

   390 1170 1170 390

If we now look at carefully at these groups you can distinguish three types of groups:

- ``396 1188 396 1188``
- ``1188 396 1188 396``

#. The first group is defined by a low 1th and low 3rd pulse pulse, hence we call it low.
#. The second group has a high 1st and 3rd pulse, hence we call it high.

We then get the following output:

.. code-block:: console

   011100000000

Each (group) of numbers has a specific meaning:

- ID: 1 till 3
- State: 8 (state)

.. code-block:: console

   x 111 xxxx 0 xxx

- The ``ID`` is defined as a binary number. To calculate the ID we subtract the binary represented decimal from 7. In this case the binary ``111`` is equal to the decimal 7. That means this code represents ID 0.
- The ``State`` defines whether a devices needs to be turned On or Off

So this code represents:

- SystemCode: 0
- State: Off