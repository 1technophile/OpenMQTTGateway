.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

Label
=====

+------------------+-------------+
| **Feature**      | **Support** |
+------------------+-------------+
| Sending          | |yes|       |
+------------------+-------------+
| Receiving        | |no|        |
+------------------+-------------+
| Config           | |yes|       |
+------------------+-------------+

.. rubric:: Supported Brands

*None*

.. rubric:: Sender Arguments

.. code-block:: console

   -i --id=id               control a device with this id
   -l --label=label         show a specific label
   -c --color=color         give the label a specific color

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "label": {
         "protocol": [ "generic_label" ],
         "id": [{
           "id": 100
         }],
         "label": "test1234",
         "color": "red"
       }
     },
     "gui": {
       "label": {
         "name": "Label",
         "group": [ "Living" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 99999       |
+------------------+-----------------+
| label            | *any value*     |
+------------------+-----------------+
| color            | *any color*     |
+------------------+-----------------+

.. note::

   Please notice that the label color is not validated in any way. The color information is just forwarded to the GUIs as is. So it could be that some GUIs do not support certain color naming. Using hex colors is therefore the safest, e.g. #000000.
