.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Secudo
======

.. versionadded:: 8.0

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

+-----------------------+----------------+
| **Brand**             | **Protocol**   |
+-----------------------+----------------+
| Secudo                | secudo         |
+-----------------------+----------------+
| FlammEx               | secudo         |
+-----------------------+----------------+

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "alarm": {
         "protocol": [ "secudo" ],
         "id": [{
           "id": 493
         }],
         "state": "alarm"
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 1023        |
+------------------+-----------------+
| state            | alarm           |
+------------------+-----------------+

.. rubric:: Optional Settings

*None*

.. rubric:: Protocol

This protocol sends 26 pulses like this:

.. code-block:: console

   314 314 628 628 314 314 628 314 628 628 314 314 628 314 628 314 628 314 628 628 314 628 314 314 628 10676

The first pulse is the ``header`` and the last pulse is the ``footer``.
These are meant to identify the pulses as genuine.
We don't use them for further processing.
The next step is to transform the remaining pulses into 12 groups of two pulses (and thereby dropping the ``header`` and ``footer`` pulses):

.. code-block:: guess

   314 628
   628 314
   314 628
   314 628
   628 314
   314 628
   314 628
   314 628
   314 628
   628 314
   628 314
   314 628

If we now look carefully at these groups you can distinguish two types of groups:

#. 314 628
#. 628 314

So the first group is defined by a high 2nd and the second group by a high 1st pulse.
So we take either of these two pulses to define a 0 or a 1.
In this case we say a high 2nd pulse means a 1 and a low 2nd pulse means a 0.
We then get the following output:

.. code-block:: guess

   101101111001

The first 10 bits correspond to the setting of the ten dip switches inside the smoke sensor.
If a dip switch is ON, this corresponds to 1 and 0 otherwise.
The meaning of the two last bits is unknown.
When reversing the first 10 bits because the first bits are least signifant we get the smoke sensors ID as binary number

.. code-block:: guess

   0111101101

which is 493 as decimal number.
This means that the 2nd, 5th and 10th dip switch is OFF and all others are ON.
