Operators
=========

.. rubric:: Arithmetic

+--------------+-------------------------------------------------+--------------------------+
| **Operator** | **Description**                                 | **Example**              |
+--------------+-------------------------------------------------+--------------------------+
| \+           | Adds two operands.                              | 10 + 20 = 30             |
+--------------+-------------------------------------------------+--------------------------+
| −            | Subtracts second operand from the first.        | 10 − 20 = -10            |
+--------------+-------------------------------------------------+--------------------------+
| \*           | Multiplies both operands.                       | 10 * 20 = 200            |
+--------------+-------------------------------------------------+--------------------------+
| /            | Divides numerator by de-numerator.              | | 10 / 20 = 2            |
|              |                                                 | | 10 / 3 = 3.333...      |
+--------------+-------------------------------------------------+--------------------------+
| \\ [#f1]_    | Integer divides numerator by de-numerator.      | 10 \\ 3 = 3              |
+--------------+-------------------------------------------------+--------------------------+
| %            | Remainder after an integer division.            | | 20 % 10 = 0            |
|              |                                                 | | 11 % 3 = 2             |
+--------------+-------------------------------------------------+--------------------------+

.. [#f1] The backslash is also used as an ``escape`` character. So if you want to use the intdivide operator inside a rule you must write your rulses like this:

         .. code-block:: json

            {
               "rules": {
                  "rule": "IF 3 \\ 2 = 1 THEN switch DEVICE light TO on",
                  "active": 1
               }
            }

.. rubric:: Relational

+--------------+-------------------------------------------------+--------------------------+
| **Operator** | **Description**                                 | **Example**              |
+--------------+-------------------------------------------------+--------------------------+
| ==           | | Checks if the first and second (nummeric)     | | 20 == 10 = false       |
|              | | operand are equal.                            | | 10 == 10 = true        |
+--------------+-------------------------------------------------+--------------------------+
| !=           | | Checks if the first and second (nummeric)     | | 20 != 10 = true        |
|              | | operand are unequal.                          | | 10 != 10 = false       |
+--------------+-------------------------------------------------+--------------------------+
| IS           | | Checks if the first and second (character)    | | a IS b = false         |
|              | | operand are equal.                            | | a IS a = true          |
+--------------+-------------------------------------------------+--------------------------+
| >=           | | Checks if the first operand is greater or     | | 20 >= 10 = true        |
|              | | equal to the second operand.                  | | 10 >= 20 = false       |
|              |                                                 | | 10 >= 10 = true        |
+--------------+-------------------------------------------------+--------------------------+
| >            | | Checks if the first operand is greater than   | | 20 > 10 = true         |
|              | | the second operand.                           | | 10 > 20 = false        |
|              |                                                 | | 10 > 10 = false        |
+--------------+-------------------------------------------------+--------------------------+
| <=           | | Checks if the first operand is smaller or     | | 20 <= 10 = false       |
|              | | equal to the second operand.                  | | 10 <= 20 = true        |
|              |                                                 | | 10 <= 10 = true        |
+--------------+-------------------------------------------------+--------------------------+
| <            | | Checks if the first operand is smaller than   | | 20 < 10 = false        |
|              | | the second operand.                           | | 10 < 20 = true         |
|              |                                                 | | 10 < 10 = false        |
+--------------+-------------------------------------------------+--------------------------+

.. versionadded:: 8.0 ISNOT operator added

.. deprecated:: 8.1.0

+--------------+-------------------------------------------------+--------------------------+
| **Operator** | **Description**                                 | **Example**              |
+--------------+-------------------------------------------------+--------------------------+
| ISNOT        | | Checks if the first and second (character)    | | a ISNOT b = true       |
|              | | operand are equal.                            | | a ISNOT a = false      |
+--------------+-------------------------------------------------+--------------------------+

.. rubric:: Logical

+--------------+-------------------------------------------------+--------------------------+
| **Operator** | **Description**                                 | **Example**              |
+--------------+-------------------------------------------------+--------------------------+
| AND          | | Checks if the first and second operands are   | | 1 AND 1 = true         |
|              | | true.                                         | | 1 AND 0 = false        |
|              | | - values greater of smaller then 0 are true   | | foo AND false = true   |
|              | | - strings equals to ``0`` are false           | | true AND true = true   |
|              | | - strings other than ``0`` are true           | | false AND false = true |
+--------------+-------------------------------------------------+--------------------------+
| OR           | | Checks if the first or second operands are    | | 0 OR 0 = false         |
|              | | true.                                         | | 1 OR 0 = true          |
|              | | - values greater of smaller then 0 are true   | | foo OR false = true    |
|              | | - strings equals to ``0`` are false           | | true OR true = true    |
|              | | - strings other than ``0`` are true           | | false OR false = true  |
+--------------+-------------------------------------------------+--------------------------+

.. rubric:: Other

.. versionadded:: 8.1.0

+----------------+-------------------------------------------------+--------------------------+
| **Operator**   | **Description**                                 | **Example**              |
+----------------+-------------------------------------------------+--------------------------+
| | .            | Concatenates two strings                        | | a . b == 'ab'          |
| | (single dot) |                                                 | | a . ' ' . b == 'a b'   |
|                |                                                 | | RANDOM(1, 2) . '34'    |
+----------------+-------------------------------------------------+--------------------------+

