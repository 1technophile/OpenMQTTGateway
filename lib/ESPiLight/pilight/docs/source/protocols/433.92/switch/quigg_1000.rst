.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Globaltronics Quigg GT-1000
===========================

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

+-------------------------------+---------------+
| **Brand**                     | **Protocol**  |
+-------------------------------+---------------+
| Globaltronics GmbH GT-1000 RC | quigg_gt1000  |
+-------------------------------+---------------+
| Globaltronics GmbH GT-FSI-08  | quigg_gt1000  |
+-------------------------------+---------------+
| Silvercrest AAA3680-A IP20 DE | quigg_gt1000  |
+-------------------------------+---------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -t   send an on signal to a device
   -f   send an off signal to a device
   -i   control the device with this system id (1 ... 15)
   -u   control the device with this unit code (0 ... 3)
   -a   control all switches in the given group
   -s   control all switches in all groups

   The next two parameters are optional:

   -n   specify the code sequence to use (0 ... 3)
   -s   specify the super codes (0 ... 7)

   If the specified sequence does not exists, sequence 0 will be used instead.
   If not specified a sequence will be selected at random.

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "quigg_gt1000" ],
         "id": [{
           "id": 15,
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
| id               | 0 - 15          |
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

.. rubric:: Examples

.. code-block:: console

   pilight-send -p quigg_gt1000 -i [0..15] -u [0..3] [-t|-f] {-n [0..3]}

Switch on or off the specified switch (-u) belonging to group (-i) optionally using specified codeseq (-n)

.. code-block:: console

   pilight-send -p quigg_gt1000 -i [0..15] -a [-t|-f] {-n [0..3]}

Switch on or off all switches belonging to group (-i) optionally using specified codeseq (-n)

.. code-block:: console

   pilight-send -p quigg_gt1000 -s [-t|-f] {-n [0..7]}

Switch on or off all compatible switches optionally using specified codeseq (-n)

When using the Web-GUI use "unit": 4 in the config.json file for the master switch ("all": 1) and "id”: 16 with "unit”: 5 for the super code switch ("super”: 1).
