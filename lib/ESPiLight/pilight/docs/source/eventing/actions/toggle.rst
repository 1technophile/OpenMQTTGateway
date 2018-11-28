.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Toggle
======

.. rubric:: Description

Toggles a device between state X and state Y. This action is especially made for ``one-shot`` devices such as a remote control button where a normal separate 'switch-on' and separate 'switch-off' rule doesn't apply. This action will check what the current state of a device is, and will change it to the non-current state.

.. rubric:: Options

+----------+------------------+---------------------+---------------------------------------------------+
| **Name** | **Required**     | **Multiple Values** | **Description**                                   |
+----------+------------------+---------------------+---------------------------------------------------+
| DEVICE   | |yes|            | |yes|               | Device(s) to change                               |
+----------+------------------+---------------------+---------------------------------------------------+
| BETWEEN  | |yes|            | |no|                | State 1                                           |
+----------+------------------+---------------------+---------------------------------------------------+
| AND      | |no|             | |no|                | State 2                                           |
+----------+------------------+---------------------+---------------------------------------------------+

.. rubric:: Example

.. code-block:: console

   IF 1 == 1 THEN toggle DEVICE television BETWEEN on AND off