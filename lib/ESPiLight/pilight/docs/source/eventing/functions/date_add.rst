.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

date_add
========

.. rubric:: Description

Calculations with date and time values.

.. rubric:: Arguments

+----------+------------------+------------------------------------------+
| **#**    | **Required**     | **Description**                          |
+----------+------------------+------------------------------------------+
| 1        | |yes|            | | A datetime device or a datetime string |
|          |                  | | The datetime string must be in the     |
|          |                  | | format YYYY-MM-DD HH:MM:SS.            |
+----------+------------------+------------------------------------------+
| 2        | |yes|            | | A number and unit. The number can be   |
|          |                  | | either positive or negative.           |
|          |                  | | The unit can be                        |
|          |                  | | - SECOND                               |
|          |                  | | - MINUTE                               |
|          |                  | | - HOUR                                 |
|          |                  | | - DAY                                  |
|          |                  | | - MONTH                                |
|          |                  | | - YEAR                                 |
+----------+------------------+------------------------------------------+

.. rubric:: Return value

The returned datetime string is always in the format ``%Y-%m-%d %H:%M:%S``. So in case of the third example, the output will be ``"2016-01-01 00:00:00"``.

.. rubric:: Examples

.. code-block:: console

   IF DATE_ADD(datetime, +1 HOUR) == ...
   IF DATE_ADD(datetime, -1 DAY) == ...
   IF DATE_ADD(2015-12-31, 23:59:59, +1 SECOND) == ...
   IF DATE_ADD(datetime, RANDOM(-3, +3) DAY) == ...
   IF DATE_FORMAT(DATE_ADD(2015-01-01 21:00:00, RANDOM(0, 120) MINUTE), \"%Y-%m-%d %H:%M:%S\", %H.%M) == ...

.. versionchanged:: 8.1.0

.. code-block:: console

   IF DATE_ADD(datetime, '+1 HOUR') == ...
   IF DATE_ADD(datetime, '-1 DAY') == ...
   IF DATE_ADD('2015-12-31, 23:59:59', '+1 SECOND') == ...
   IF DATE_ADD(datetime, RANDOM(-3, +3) . '  DAY') == ...
   IF DATE_FORMAT(DATE_ADD('2015-01-01 21:00:00', RANDOM(0, 120) . ' MINUTE'), '%Y-%m-%d %H:%M:%S', %H.%M) == ...
