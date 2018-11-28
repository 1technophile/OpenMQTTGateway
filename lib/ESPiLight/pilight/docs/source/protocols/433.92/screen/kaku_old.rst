.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

KlikAanKlikUit (Old)
====================

+------------------+-------------+
| **Feature**      | **Support** |
+------------------+-------------+
| Sending          | |no|        |
+------------------+-------------+
| Receiving        | |yes|       |
+------------------+-------------+
| Config           | |yes|       |
+------------------+-------------+

.. rubric:: Supported Brands

+------------------+------------------+
| **Brand**        | **Protocol**     |
+------------------+------------------+
| KlikAanKlikUit   | kaku_screen_old  |
+------------------+------------------+

.. rubric:: Sender Arguments

.. code-block:: console

   -t --up           send an up signal
   -f --down         send an down signal
   -u --unit=unit    control a device with this unit code
   -i --id=id        control a device with this id

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "screen": {
         "protocol": [ "kaku_screen_old" ],
         "id": [{
           "id": 16,
           "unit": 0
         }],
         "state": "down"
        }
     },
     "gui": {
       "screen": {
         "name": "Screen",
         "group": [ "Living" ],
         "media": [ "all" ]
       }
     }
   }


+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 31          |
+------------------+-----------------+
| unit             | 0 - 15          |
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

.. rubric:: Notes


The old KlikAanKlikUit devices work setting two wheels: group (0-H) and device number (0-16). To control a device from pilight you need to send the correct group and unit id, these do not correspond 1:1.

A-1 on the unit corresponds with unit 0 id 0 in pilight A-2 on the unit corresponds with unit 0 id 1 in pilight etc.

.. rubric:: Protocol

This protocol sends 50 pulses like this

.. code-block:: guess

   295 1180 295 1180 295 1180 1180 295 295 1180 295 1180 295 1180 1180 295 295 1180 1180 295 295 1180 1180 295 295 1180 1180 295 295 1180 1180 295 295 1180 295 1180 295 1180 1180 295 295 1180 1180 295 295 1180 1180 295 295 11210

It has no ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine, and the protocol also has some bit checks to filter false positives. We don't use them for further processing. The next step is to transform this output into 12 groups of 4 pulses (and thereby dropping the ``footer`` pulses).

.. code-block:: guess

   295 1180 295 1180
   295 1180 1180 295
   295 1180 295 1180
   295 1180 1180 295
   295 1180 1180 295

   295 1180 1180 295
   295 1180 1180 295
   295 1180 1180 295
   295 1180 295 1180
   295 1180 1180 295

   295 1180 1180 295
   295 1180 1180 295

   295 11210

If we now look at carefully at these groups you can distinguish two types of groups:

#. 295 1180 1180 295
#. 295 1180 295 1180

So the first group is defined by a high 3th pulse and the second group has a low 3rd pulse. In this case we say a high 3rd pulse means a 0 and a high 3rd pulse means a 1. We then get the following output:

.. code-block:: guess

   10100 00010 0 0

Each (group) of numbers has a specific meaning:

- Unit: 0 till 5 (inversed)
- ID: 6 till 10 (inversed)
- Fixed: 11 (always 0)
- State: 12 (inversed)

.. code-block:: guess

   10100 00010 0 0

- The ``Unit`` is defined as a binary number
- The ``ID`` is defined as a binary number
- The ``Fixed`` is always 0
- The ``State`` defines whether a devices needs to be go Up or Down

So this code represents:

- Unit: 20
- ID: 1
- Fixed: always 0
- State: Down

Another example:

- Unit: 0
- ID: 4
- Fixed: always 0
- State: Up

.. code-block:: guess

   00000 00100 0 1

Furthermore the protocol filters out false positives by checking if:

#. Every 1st bit of the first 12 groups of 4 bits is always LOW (0)
#. 2nd bit of the first 12 groups of 4 bits is always HIGH (1)
#. 3rd and 4th bit of the first 12 groups of 4 bits are different (NOT EQUAL)
#. Bits 49 and 50 are LOW (0) and HIGH (1) respectively (fixed footer)

This makes the protocol more accurate because it will respond rarely now when sartano commands are sent.