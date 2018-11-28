.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Impuls
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
| Impuls               | impuls           |
+----------------------+------------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -s --systemcode=systemcode    control a device with this systemcode
   -u --programcode=programcode  control a device with this programcode
   -t --on                       send an on signal
   -f --off                      send an off signal

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "impuls" ],
         "id": [{
           "programcode": 8,
           "unitcode": 12
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

.. rubric:: Notes

The default settings for a set of Impuls switches are:

#. SystemCode: 11111 and ProgramCode: 10000
#. ProgramCode: 01000
#. ProgramCode: 00100

This translates into the following pilight systemcode and unitcode:

#. systemcode: 31, programcode: 1
#. systemcode: 31, programcode: 2
#. systemcode: 31, programcode: 4

.. rubric:: Protocol

This protocol sends 50 pulses like this

.. code-block:: console

   885 295 885 295 885 295 885 295 885 295 885 295 295 885 885 295 295 885 885 295 295 885 295 885 295 885 885 295 295 885 295 885 295 885 885 295 295 885 885 295 295 885 885 295 295 885 295 885 295 9735

It has no ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine, and the protocol also has some bit checks to filter false positives. We don't use them for further processing. The next step is to transform this output into 12 groups of 4 pulses (and thereby dropping the ``footer`` pulses).

.. code-block:: console

   885 295 885 295
   885 295 885 295
   885 295 885 295
   295 885 885 295
   295 885 885 295

   295 885 295 885
   295 885 885 295
   295 885 295 885
   295 885 885 295
   295 885 885 295

   295 885 885 295
   295 885 295 885

   295 9735

If we now look at carefully at these groups you can distinguish three types of groups:

#. ``295 885 295 885``
#. ``885 295 885 295``
#. ``295 885 885 295``

- The first group is defined by a low 1th and low 3rd pulse pulse, hence we call it low.
- The second group has a high 1st and 3rd pulse, hence we call it med.
- The final group has a low 1st and high 3rd pulse, hence we call it high.

We then get the following output:

.. code-block:: console

   MMMHH LHLHH H L

All M's and H's can be translated to 1. All L's can be translated to 0.

Each (group) of numbers has a specific meaning:

- Unit: 0 till 5
- ID: 6 till 10
- Check: 11 (inverse state)
- State: 12 (state)

.. code-block:: console

   11100 10100 1 0

- The ``SystemCode`` is defined as a binary number
- The ``ProgramCode`` is defined as a binary number
- The ``Check`` is always inverse state
- The ``State`` defines whether a devices needs to be turned On or Off

So this code represents:

- SystemCode: 7
- ProgramCode: 5
- Check: Inverse state
- State: Off

Another example:

- Unit: 0
- ID: 4
- Fixed: Inverse state
- State: On

.. code-block:: console

   00000 00100 0 1