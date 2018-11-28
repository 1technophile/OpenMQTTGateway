.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

date_format
===========

.. rubric:: Description

Reformat date and time values

.. rubric:: Arguments

+----------+------------------+------------------------------------------+
| **#**    | **Required**     | **Description**                          |
+----------+------------------+------------------------------------------+
| **Using two parameters**                                               |
+----------+------------------+------------------------------------------+
| 1        | |yes|            | A datetime device                        |
+----------+------------------+------------------------------------------+
| 2        | |yes|            | Output format (see below)                |
+----------+------------------+------------------------------------------+
| **Using three parameters**                                             |
+----------+------------------+------------------------------------------+
| 1        | |yes|            | A datetime string                        |
+----------+------------------+------------------------------------------+
| 2        | |yes|            | Input format (see below)                 |
+----------+------------------+------------------------------------------+
| 3        | |yes|            | Output format (see below)                |
+----------+------------------+------------------------------------------+

.. note:: **Input / output**

   Consider that the flexibility of the function depends on the input. If you input just ``%H:%M:%S``, the DATE_FORMAT can't output a format like this ``%Y-%m-%d``, because the function didn't receive any date information. It only received time information. So, DATE_FORMAT can't be used to exterpolate information that's not present.

.. rubric:: Return value

The function will output the exact format as requested in the last (second or third argument). There are a few things you must consider when working with the DATE_FORMAT function. The types of operators you can use with DATE_FORMAT depends on the output format requested. If you want to compare the output format ``%c`` you can only use the ``IS`` operator, because the final output will be interpreted as a string value. If you want to compare ``%H.%M``, you will need numeric operators like ``==``, ``>``, or ``<``, because the output will be evaluated as a number.

Also consider the usage of quotes. The flexibility in which DATE_FORMAT can format date and time values isn't always accepted by pilight. For example: ``DATE_FORMAT(datetime, %c)`` outputs ``Fri Apr 24 11:35:36 2015`` which pilight subsequently can't handle. If you want to output string like this, use quotes ``DATE_FORMAT(datetime, "%c")``, which will output as ``"Fri Apr 24 11:35:36 2015"``.

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
   IF DATE_ADD(2015-12-31, 23:59:59, '+1 SECOND') == ...
   IF DATE_ADD(datetime, RANDOM(-3, +3) . ' DAY') == ...
   IF DATE_FORMAT(DATE_ADD('2015-01-01 21:00:00', RANDOM(0, 120) . ' MINUTE'), '%Y-%m-%d %H:%M:%S', %H.%M) == ...

.. note:: **DATE_ADD as an argument**

When you use DATE_ADD as the first argument, you should consider the return value of DATE_ADD, because that's what DATE_FORMAT will eventually receive. The output format of DATE_ADD is ``"%Y-%m-%d %H:%M:%S"``, so the input format for DATE_FORMAT should be the same.

.. rubric:: Formatting characters

+---------------+------------------------------------------------------------------------+--------------------------+
| **Specifier** | **Replaced by**                                                        | **Example**              |
+---------------+------------------------------------------------------------------------+--------------------------+
| %a            | Abbreviated weekday name [#f1]_                                        | Thu                      |
+---------------+------------------------------------------------------------------------+--------------------------+
| %A            | Full weekday name [#f1]_                                               | Thursday                 |
+---------------+------------------------------------------------------------------------+--------------------------+
| %b %h         | Abbreviated month name [#f1]_                                          | Aug                      |
+---------------+------------------------------------------------------------------------+--------------------------+
| %B            | Full month name [#f1]_                                                 | August                   |
+---------------+------------------------------------------------------------------------+--------------------------+
| %c            | Date and time representation [#f1]_                                    | Thu Aug 23 14:55:02 2001 |
+---------------+------------------------------------------------------------------------+--------------------------+
| %C            | Year divided by 100 and truncated to integer (00-99)                   | 20                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %d            | Day of the month, zero-padded (01-31)                                  | 23                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %D            | Short MM/DD/YY date, equivalent to %m/%d/%y                            | 08/23/01                 |
+---------------+------------------------------------------------------------------------+--------------------------+
| %e            | Day of the month, space-padded (1-31)	                                 | 23                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %F            | Short YYYY-MM-DD date, equivalent to %Y-%m-%d                          | 2001-08-23               |
+---------------+------------------------------------------------------------------------+--------------------------+
| %g            | Week-based year, last two digits (00-99)                               | 01                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %G            | Week-based year                                                        | 2001                     |
+---------------+------------------------------------------------------------------------+--------------------------+
| %g            | Week-based year                                                        | 2001                     |
+---------------+------------------------------------------------------------------------+--------------------------+
| %H            | Hour in 24h format (00-23)                                             | 14                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %I            | Hour in 12h format (01-12)                                             | 02                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %j            | Day of the year (001-366)                                              | 235                      |
+---------------+------------------------------------------------------------------------+--------------------------+
| %m            | Month as a decimal number (01-12)                                      | 08                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %M            | Minute (00-59)                                                         | 55                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %n            | New-line character ('\n')                                              |                          |
+---------------+------------------------------------------------------------------------+--------------------------+
| %p            | AM or PM designation                                                   | PM                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %r            | 12-hour clock time [#f1]_                                              | 02:55:02 pm              |
+---------------+------------------------------------------------------------------------+--------------------------+
| %R            | 24-hour HH:MM time, equivalent to %H:%M                                | 14:55                    |
+---------------+------------------------------------------------------------------------+--------------------------+
| %S            | Second (00-61)                                                         | 02                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %t            | Horizontal-tab character ('\t')                                        |                          |
+---------------+------------------------------------------------------------------------+--------------------------+
| %T            | ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S                | 14:55:02                 |
+---------------+------------------------------------------------------------------------+--------------------------+
| %u            | ISO 8601 weekday as number with Monday as 1 (1-7)                      | 4                        |
+---------------+------------------------------------------------------------------------+--------------------------+
| %U            | Week number with the first Sunday as the first day of week one (00-53) | 33                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %V            | ISO 8601 week number (01-53)                                           | 34                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %w            | Weekday as a decimal number with Sunday as 0 (0-6)                     | 4                        |
+---------------+------------------------------------------------------------------------+--------------------------+
| %W            | Week number with the first Monday as the first day of week one (00-53) | 34                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %x            | Date representation [#f1]_                                             | 08/23/01                 |
+---------------+------------------------------------------------------------------------+--------------------------+
| %X            | Time representation [#f1]_                                             | 14:55:02                 |
+---------------+------------------------------------------------------------------------+--------------------------+
| %y            | Year, last two digits (00-99)                                          | 01                       |
+---------------+------------------------------------------------------------------------+--------------------------+
| %Y            | Year                                                                   | 2001                     |
+---------------+------------------------------------------------------------------------+--------------------------+
| %z            | | ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100)        | +100                     |
|               | | If timezone cannot be determined, no characters                      |                          |
+---------------+------------------------------------------------------------------------+--------------------------+
| %Z            | | Timezone name or abbreviation [#f1]_                                 | CDT                      |
|               | | If timezone cannot be determined, no characters                      |                          |
+---------------+------------------------------------------------------------------------+--------------------------+
| %%            | A ``%`` sign                                                           | %                        |
+---------------+------------------------------------------------------------------------+--------------------------+

Copied from http://www.cplusplus.com/reference/ctime/strftime/

.. [#f1] These specifiers are locale-dependent.