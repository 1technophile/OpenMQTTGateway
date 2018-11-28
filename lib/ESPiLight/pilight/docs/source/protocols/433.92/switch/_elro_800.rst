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
| Elro 800 Series      | elro_800_switch  |
+----------------------+------------------+
| Brennenstuhl         | brennenstuhl     |
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
         "protocol": [ "elro_800_switch" ],
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
| systemcode       | 0 - 1023        |
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

.. Note:: Workaround for 800 Series

   1. Set your remote DIP switches like this off|off|on|off (AB660 Code) if possible
   2. Now look for systemcode and unitcode with pilight-receive
   3. Setup the config.json with the received results
   4. Get the switches in learning mode and push the buttons on the webgui accordingly to the unit.
   5. This should work until there is a protocol for the 800 series.


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

If we now look at carefully at these groups you can distinguish three types of groups:

- ``320 960 320 960``
- ``320 960 960 320``

So the first group is defined by a high 4th pulse and the second group has a low 4th pulse. In this case we say a high 4th pulse means a 1 and a low 4th pulse means a 0. We then get the following output:

.. code-block:: console

   11111 10000 1 0

Each (group) of numbers has a specific meaning:

- SystemCode: 0 till 5
- UnitCode: 6 till 10
- State: 11
- Check: 12 (inverse state)


- The ``SystemCode`` is defined as a binary number
- The ``UnitCode``  is defined as a binary number
- The ``State`` defines whether a devices needs to be turned On or Off
- The ``Check`` defines whether a devices needs to be turned On or Off (but is inverse)

So this code represents:

- SystemCode: 31
- UnitCode: 1
- State: On
- Check: On (inverse state)

.. code-block:: console

   00000 00100 0 1

Another example:

- SystemCode: 0
- UnitCode: 4
- State: Off
- Check: Off (inverse state)

Furthermore the protocol filters out false positives by checking if:

- Every 1st bit of the first 12 groups of 4 bits is always LOW (0)
- 2nd bit of the first 12 groups of 4 bits is always HIGH (1)
- 3rd and 4th bit of the first 12 groups of 4 bits are different (NOT EQUAL)
- Bits 49 and 50 are LOW (0) and HIGH (1) respectively (fixed footer)

This makes the protocol more accurate because it will respond less when arctech_old commands are sent.

The numeric mapping of a button on the remote control is binary counted, as follows:

- A = 1
- B = 2
- C = 4
- D = 8
- E = 16

All possible combinations of dip switches on the socket can be used with pilight. For instance, if you set the switches that correspond to A and E, the socket will react on unit code “A+E”: 17 (1+16). By pushing A and E simultaneously on the remote, you can also switch the corresponding socket. However, this may also trigger sockets that are listening to A and/or E if you don't push both buttons at exactly the same time.
