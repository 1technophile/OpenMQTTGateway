.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

KlikAanKlikUit
==============

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

+------------------+--------------+
| **Brand**        | **Protocol** |
+------------------+--------------+
| KlikAanKlikUit   | kaku_contact |
+------------------+--------------+

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "contact": {
         "protocol": [ "kaku_contact" ],
         "id": [{
           "id": 100,
           "unit": 0
         }],
         "state": "closed"
        }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 67108863    |
+------------------+-----------------+
| code             | 0 - 15          |
+------------------+-----------------+
| state            | opened / closed |
+------------------+-----------------+

.. rubric:: Optional Settings

*None*

.. rubric:: Protocol

This protocol sends 148 pulses like this

.. code-block:: guess

   294 2646 294 294 294 1176 294 294 294 1176 294 1176 294 294 294 294 294 1176 294 1176 294 294 294 294 294 1176 294 1176 294 294 294 1176 294 294 294 1176 294 294 294 294 294 1176 294 1176 294 294 294 1176 294 294 294 294 294 1176 294 294 294 1176 294 1176 294 294 294 294 294 1176 294 294 294 1176 294 1176 294 294 294 294 294 1176 294 294 294 1176 294 294 294 1176 294 1176 294 294 294 294 294 1176 294 294 294 1176 294 1176 294 294 294 294 294 1176 294 294 294 1176 294 1176 294 294 294 1176 294 294 294 294 294 1176 294 294 294 1176 294 1176 294 294 294 294 294 1176 294 1176 294 294 294 1176 294 294 294 1176 294 294 294 9996

The first 2 pulses are the ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine. We don't use them for further processing. The next step is to transform this output into 36 groups of 4 pulses (and thereby dropping the ``header`` and ``footer`` pulses).

.. code-block:: guess

   294 294 294 1176
   294 294 294 1176
   294 1176 294 294
   294 294 294 1176
   294 1176 294 294
   294 294 294 1176
   294 1176 294 294
   294 1176 294 294
   294 1176 294 294
   294 294 294 1176
   294 1176 294 294
   294 1176 294 294
   294 294 294 1176
   294 294 294 1176
   294 1176 294 294
   294 294 294 1176
   294 294 294 1176
   294 1176 294 294
   294 294 294 1176
   294 294 294 1176
   294 294 294 1176
   294 1176 294 294
   294 294 294 1176
   294 294 294 1176
   294 1176 294 294
   294 294 294 1176
   294 294 294 1176
   294 1176 294 294
   294 1176 294 294
   294 294 294 1176
   294 294 294 1176
   294 1176 294 294
   294 294 294 1176
   294 1176 294 294
   294 1176 294 294
   294 1176 294 294

If we now look at carefully at these groups you can distinguish two types of groups:

#. 294 1176 294 294
#. 294 294 294 1176

So the first group is defined by a high 2nd and the second group has a high 4th pulse. So we take either of these two pulses to define a 0 or a 1. In this case we say a high 2nd pulse means a 1 and a low 2nd pulse means a 0. We then get the following output:

.. code-block:: guess

   001010111011001001000100100110010111

Each (group) of numbers has a specific meaning:

- ID: 0 till 25
- All: 26
- State: 27
- Unit: 28 till 31
- Unknown: 32 till 35

.. code-block:: guess

   00101011101100100100010010 0 1 1001 0111

- The ``ID`` is defined as a binary number
- The ``All`` tells us if a code was meant for all devices with the same ID
- The ``State`` defines whether a devices is opened or closed
- The ``Unit`` is also defined as a binary number
- The ``Unknown`` is also defined as a binary number(bits are missing in the closed state)

So this code represents:

- ID: 11454738
- All: Single
- State: Opened
- Unit: 9
