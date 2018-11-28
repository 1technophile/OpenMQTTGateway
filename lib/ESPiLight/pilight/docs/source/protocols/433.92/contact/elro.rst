.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Elro HE Contact
===============

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
| Elro 800 Series  | elro_800_contact |
+------------------+------------------+

.. rubric:: Sender Arguments

*None*

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "contact": {
         "protocol": [ "elro_800_contact" ],
         "id": [{
           "systemcode": 31,
           "unitcode": 0
         }],
         "state": "closed"
        }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 31          |
+------------------+-----------------+
| code             | 0 - 31          |
+------------------+-----------------+
| state            | opened / closed |
+------------------+-----------------+

.. rubric:: Optional Settings

*None*

.. rubric:: Protocol

This protocol sends 148 pulses like this

.. code-block:: guess

   320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 320 960 960 320 320 960 960 320 320 960 960 320 320 960 960 320 320 960 320 960 320 960 960 320 320 9920

It has no ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine, and the protocol also has some bit checks to filter false positives. We don't use them for further processing. The next step is to transform this output into 12 groups of 4 pulses (and thereby dropping the ``footer`` pulses).

.. code-block:: guess

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

If we now look at carefully at these groups you can distinguish two types of groups:

#. 320 960 320 960
#. 320 960 960 320

So the first group is defined by a high 4th pulse and the second group has a low 4th pulse. In this case we say a high 4th pulse means a 1 and a low 4th pulse means a 0. We then get the following output:

.. code-block:: guess

   11111 10000 1 0

Each (group) of numbers has a specific meaning:

- SystemCode: 0 till 5
- UnitCode: 6 till 10
- State: 11
- Check: 12 (inverse state)

.. code-block:: guess

   11111 10000 1 0

- The ``SystemCode`` is defined as a binary number
- The ``UnitCode`` is defined as a binary number
- The ``State`` defines whether a devices needs to be turned On or Off
- The ``Check`` defines whether a devices needs to be turned On or Off (but is inverse)

So this code represents:

- SystemCode: 31
- UnitCode: 1
- State: Opened
- Check: Opened (inverse state)

Another example:

- SystemCode: 0
- UnitCode: 4
- State: Closed
- Check: Closed (inverse state)

.. code-block:: guess

   00000 00100 0 1

Furthermore the protocol filters out false positives by checking if:

- Every 1st bit of the first 12 groups of 4 bits is always LOW (0)
- 2nd bit of the first 12 groups of 4 bits is always HIGH (1)
- 3rd and 4th bit of the first 12 groups of 4 bits are different (NOT EQUAL)
- Bits 49 and 50 are LOW (0) and HIGH (1) respectively (fixed footer)

This makes the protocol more accurate because it will respond less when arctech_old commands are sent.