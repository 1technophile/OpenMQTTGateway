Rules
=====

.. warning::

   Make sure pilight is not running before editing your configuration or else all changes will be lost.

Since pilight version 6, eventing has been introduced. To use events, you need to first define rules inside the pilight configuration.

The basic pilight rule structure for the configuration looks like this:

.. code-block:: json
   :linenos:

   {
     "rules": {
       "rule_name": {
         "rule": "IF switch.state IS on THEN switch DEVICE switch.state TO off",
         "active": 1
       }
     }
   }

As you can see, a rules object consists of various elements:

#. Each rule is placed in its own object with a unique identifier.
#. Each rule consists of the actual rule placed in the rule field.
#. Each rule consists of an active field that tells pilight if the rule is active or not.

You can of course define as many rules as you like.