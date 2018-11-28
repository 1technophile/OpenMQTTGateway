.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

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
| Cogex                | cogex            |
+----------------------+------------------+
| Dúwi Terminal        | duwi             |
+----------------------+------------------+
| Eurodomest           | kaku_switch_old  |
+----------------------+------------------+
| Intertechno          | intertechno_old  |
+----------------------+------------------+
| Dúwi Terminal        | duwi             |
+----------------------+------------------+
| KlikAanKlikUit       | kaku_switch_old  |
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
         "protocol": [ "kaku_switch_old" ],
         "id": [{
           "id": 10,
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
| id               | 0 - 31          |
+------------------+-----------------+
| unit             | 0 - 15          |
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

.. rubric:: Notes

The old KlikAanKlikUit devices work setting two wheels: group (0-H) and device number (0-16). To control a device from pilight you need to send the correct group and unit id, these do not correspond 1:1.

A-1 on the unit corresponds with unit 0 id 0 in pilight A-2 on the unit corresponds with unit 0 id 1 in pilight etc.

.. rubric:: Protocol

This protocol sends 50 pulses like this

.. code-block:: console

   295 1180 295 1180 295 1180 1180 295 295 1180 295 1180 295 1180 1180 295 295 1180 1180 295 295 1180 1180 295 295 1180 1180 295 295 1180 1180 295 295 1180 295 1180 295 1180 1180 295 295 1180 1180 295 295 1180 1180 295 295 11210

It has no ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine, and the protocol also has some bit checks to filter false positives. We don't use them for further processing. The next step is to transform this output into 12 groups of 4 pulses (and thereby dropping the ``footer`` pulses).

.. code-block:: console

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

- ``295 1180 1180 295``
- ``295 1180 295 1180``

So the first group is defined by a high 3th pulse and the second group has a low 3rd pulse. In this case we say a high 3rd pulse means a 0 and a low 3rd pulse means a 1. We then get the following output:

.. code-block:: console

   10100 00010 0 0

Each (group) of numbers has a specific meaning:

- Unit: 0 till 5 (inversed)
- ID: 6 till 10 (inversed)
- Fixed: 11 (always 0)
- State: 12 (inversed)

.. code-block:: console

   10100 00010 0 0

- The ``Unit`` is defined as a binary number
- The ``ID`` is defined as a binary number
- The ``Fixed``  is always 0
- The ``State`` defines whether a devices needs to be turned On or Off

So this code represents:

- Unit: 20
- ID: 1
- Fixed: always 0
- State: Off

Another example:

- Unit: 0
- ID: 4
- Fixed: always 0
- State: On

.. code-block:: console

   00000 00100 0 1

Furthermore the protocol filters out false positives by checking if:

- Every 1st bit of the first 12 groups of 4 bits is always LOW (0)
- 2nd bit of the first 12 groups of 4 bits is always HIGH (1)
- 3rd and 4th bit of the first 12 groups of 4 bits are different (NOT EQUAL)
- Bits 49 and 50 are LOW (0) and HIGH (1) respectively (fixed footer)

This makes the protocol more accurate because it will respond rarely now when sartano commands are sent.

.. note:: **Eurodomest**

   The Eurodomest (Intertechno) switches listens to the kaku_switch_old protocol, but the remote doesn't send using the kaku_switch_old protocol. When using pilight-receive, the following protocols are received using the remote:

     - elro_800_contact
     - elro_800_switch
     - ehome
     - cleverwatts

   To use the Eurodomest switches with the kaku_switch_old protocol on pilight, the following steps have to be performed:

     - Hold the On/Off button until the LED flashes.
     - In a terminal window do:

       .. code-block:: console

          pilight-send -p kaku_switch_old -i <choose your own ID between 1~30> -u 0 -t

     - If everything is Ok the switch is programmed.
     - For wall-plug 2 & 3 change the value -u

