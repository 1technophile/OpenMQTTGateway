.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

random
======

.. rubric:: Description

Generate a random number.

.. rubric:: Parameters

+----------+------------------+---------------------+
| **#**    | **Required**     | **Description**     |
+----------+------------------+---------------------+
| 1        | |yes|            | Lower boundary      |
+----------+------------------+---------------------+
| 2        | |yes|            | Higher boundary     |
+----------+------------------+---------------------+

.. rubric:: Return value

A random number

.. rubric:: Examples

.. code-block:: console

   IF RANDOM(0, 10) == 5 THEN ...
   IF RANDOM(1, 100) == 10 THEN ...
   IF RANDOM(randomLow.dimlevel, randomHigh.dimlevel) == 10 THEN ...