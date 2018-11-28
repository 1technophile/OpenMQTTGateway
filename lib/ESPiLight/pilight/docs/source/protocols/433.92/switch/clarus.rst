.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Clarus
======

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
| Clarus               | clarus_switch    |
+----------------------+------------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -i --id=id             control a device with this systemcode
   -u --unit=unit         control a device with this programcode
   -t --on                send an on signal
   -f --off               send an off signal

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "clarus_switch" ],
         "id": [{
           "id": "A2",
           "unit": 10
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
| id               | A0 - F32        |
+------------------+-----------------+
| unit             | 0 - 63          |
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

   189 567 567 189 189 567 567 189 189 567 189 567 189 567 567 189 189 567 189 567 189 567 567 189 189 567 567 189 189 567 567 189 189 567 189 567 567 189 567 189 189 567 189 567 567 189 567 189 189 6426

It has no ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine, and the protocol also has some bit checks to filter false positives. We don't use them for further processing. The next step is to transform this output into 12 groups of 4 pulses (and thereby dropping the ``footer`` pulses).

.. code-block:: console

   189 567 567 189
   189 567 567 189
   189 567 189 567
   189 567 567 189
   189 567 189 567

   189 567 567 189
   189 567 567 189
   189 567 567 189
   189 567 189 567
   567 189 567 189

   189 567 189 567
   567 189 567 189

   189 6426

If we now look at carefully at these groups you can distinguish three types of groups:

#. ``189 567 567 189``
#. ``567 189 567 189``
#. ``189 567 189 567``

- The first group is defined by a low 1st and low 4rd pulse pulse, hence we call it low.
- The second group has a high 1st and 3rd pulse, hence we call it med.
- The final group has a low 1st and low 3rd pulse, hence we call it high.

We then get the following output:

.. code-block:: console

   LLHLH LLLHM HM

All L's can be translated to 0, the H's to 1's, and M's to 2.

Each (group) of numbers has a specific meaning:

- Unit: 0 till 5
- ID: 6 till 10
- State: 12 (state)

.. code-block:: console

   00101 00012 12

- The ``Unit`` is defined as a binary number
- The ``ID`` is defined as a binary number combined with a letter
- The ``State`` defines whether a devices needs to be turned On or Off

So this code represents:

- Unit: 10
- ID: A2
- State: On

Another example:

- Unit: 0
- ID: B1
- State: Off

.. code-block:: console

   00101 00021 21