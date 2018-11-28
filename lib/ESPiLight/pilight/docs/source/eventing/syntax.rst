.. role:: event-success
.. role:: event-fail

Syntax
======

- `Introduction`_
- `Rule Parsing`_
   - `Basic Structure`_
   - `True or False`_
   - `AND and OR`_
   - `Hooks`_
- `Operator`_
- `String and Concatenation`_
- `Devices`_
- `Functions`_
- `Actions`_
- `Using Device Parameters`_

Introduction
------------

The pilight eventing library allows you to automate your house by writing event rules. These rules tell pilight what should happen at a given time or at a given event. pilight event rules follow the regular mathematical conventions and because you can work with an infinite number of hooks, they can become highly complex but are also highly flexible.

As with everything inside pilight, the eventing library is also highly modular. This means that the operators, actions, and functions are external modules that can be easily extended. Developers and users do not have to touch the core of the eventing library to add additional features.

Rule Parsing
------------

There are few things that are useful to know before working with pilight event rules. When you better understand how pilight parses the rules, you can write really efficient and advanced rules.

.. _Basic Structure:
.. rubric:: Basic Structure

All rules basically consist of the same structure:

.. code-block:: guess

   IF ... THEN ...

A rule always start with the IF, followed by the rule logic, and is closed by a THEN. All actions are put after the THEN.

.. versionadded:: 8.1.0

The IF / THEN logic can be expanded by an ELSE. The actions written after the ELSE will be executed if the logic contained in the first if block is false.

.. code-block:: guess

   IF ... THEN ... ELSE ...

Or you can nest an infinite number of IF/ELSE conditions by using adding the END keyword

.. code-block:: guess

   IF ... THEN IF ... THEN ... ELSE ... END ELSE IF ... THEN ... ELSE ... END END

.. _True or False:

.. rubric:: True or False

Let us start with how pilight handles succeeding or failing conditions. pilight converts each logical mathematical and logical step into a true or false. The final condition must be true before triggering an action. If the final result is false, the action is skipped. These two examples will both succeed:

:event-success:`Succeeds`

.. code-block:: guess

   IF (1 == 1) == True THEN

:event-success:`Succeeds`

.. code-block:: guess

   IF (1 == 1) != False THEN

These two examples will both fail:

:event-fail:`Fails`

.. code-block:: guess

   IF (1 == 0) IS True THEN

:event-fail:`Fails`

.. code-block:: guess

   IF (1 != 1) IS False THEN

.. _AND and OR:
.. rubric:: AND and OR

In the first examples we only used one condition but this does not allow us to make very advanced and flexible rules. Therefore we need to be able to combine several conditions in a single rule. This is possible by combining them with AND and OR operators. Let us take a look at some basic examples:

:event-fail:`Fails`

.. code-block:: guess

   IF 1 == 1 AND 1 == 2 THEN ...

:event-success:`Succeeds`

.. code-block:: guess

   IF 1 == 1 OR 1 == 2 THEN ...

:event-fail:`Fails`

.. code-block:: guess

   IF 1 == 2 OR 1 == 1 AND 2 == 3 THEN ...

:event-success:`Succeeds`

.. code-block:: guess

   IF 1 == 1 OR 1 == 1 AND 2 == 3 THEN ...

:event-fail:`Fails`

.. code-block:: guess

   IF 1 == 2 AND 1 == 1 OR 2 == 3 THEN ...

:event-success:`Succeeds`

.. code-block:: guess

   IF 1 == 1 AND 2 == 2 OR 2 == 3 THEN ...

In the first two examples only a single AND and OR operator is used. In the rest of the examples we see multiple AND and OR operators. pilight follows standard operator associativity rules in parsing AND and OR operators.

.. _Hooks:
.. rubric:: Hooks

As we saw in our first examples, hooks can be used inside pilight rules. This can be useful to better structure and combine the various conditions of our rules. Let us create even more complex rules and see how hooks can change the outcome of a rule without changing the conditions.

:event-success:`Succeeds`

.. code-block:: guess

   IF 1 == 2 OR 2 == 3 AND 2 == 3 OR 1 == 1 THEN ...

:event-fail:`Fails`

.. code-block:: guess

   IF (1 == 2 OR 2 == 3) AND (2 == 3 OR 1 == 1) THEN ...

Operator
--------

Various mathematical operators can be used to do calculations inside our rules. A list of these operators can be found further on in this manual. pilight follows standard operator associativity rules in parsing mathematical operators. Let us just show some basic self-explanatory examples:

:event-success:`Succeeds`

.. code-block:: guess

   IF 1 + 1 == 2 THEN ...

:event-success:`Succeeds`

.. code-block:: guess

   IF 2 % 2 == 0 THEN ...

:event-success:`Succeeds`

.. code-block:: guess

   IF 1 < 10 THEN ...

:event-fail:`Fails`

.. code-block:: guess

   IF 10 / 2 == 4 THEN ...

String and Concatenation
------------------------

.. versionadded:: 8.1.0

pilight parses individual components inside a rule by looking at queues like spaces, comma's, hooks, and preserved words (e.g. actions or functions). In any case, pilight can be forces to parse any of these components as strings by enclosing them into quotes.

:event-success:`Succeeds`

.. code-block:: guess

   IF 1 + 1 == 3 - 1 THEN ...

In this case, pilight will compare the outcome of both formulas (which is 2) with each other for further parsing. We can also tell pilight to compare the same formula but now with strings:

:event-fail:`Fails`

.. code-block:: guess

   IF '1 + 1' == '3 - 1' THEN ...

The same formula parsed with two strings will now fail because both strings are not equal anymore.

It is important to understand how to enforce string parsing when working with functions. The following example with give a syntax error, because the DATE_ADD function only expects two arguments delimited by a comma when the first argument is a datetime device. However, pilight sees the ``+1`` and ``HOUR`` as two separate string that normally should be delimited by a comma. Would be add that comma, we would still call the DATE_ADD function with the wrong parameter count.

.. code-block:: guess

   IF DATE_ADD(datetime, +1 HOUR) == ...

The rule above should be written like this instead so the ``+1`` and ``HOUR`` are parsed like ``+1 HOUR``.

.. code-block:: guess

   IF DATE_ADD(datetime, '+1 HOUR') == ...

Another issue arises when strings and device values or functions need to be combined like in this example:

.. code-block:: guess

   IF ... THEN label DEVICE alarm TO the alarm was trigger at DATE_FORMAT(datetime, %Y-%m-%d)

This rule will trigger a syntax error because the TO argument only expects one string, but five strings and a function were given. To make this into one string we need to use quotes and the concatenate ``.`` operator:

.. code-block:: guess

   IF ... THEN label DEVICE alarm TO 'the alarm was trigger at' . DATE_FORMAT(datetime, %Y-%m-%d)

Another scenario in which we want to explicitly cast a keyword into a string is given in this example:

.. code-block:: guess

   IF ... THEN label DEVICE label TO on

The ``label`` device is parsed by pilight as the label action because it's a reserved keyword. pilight does allow using label as a device name when casting it as a string like this:

.. code-block:: guess

   IF ... THEN label DEVICE 'label' TO on

As shown above, keywords or operators can be parsed into strings so pilight will ignore their default meaning.

Devices
-------

pilight rules are quite useless if we cannot work with live data. This live data comes from our devices in and around the house. So let us say we have a switch called switch and we use this configured device to create a rule like this:

.. code-block:: guess

   IF switch.state IS on THEN ...

Depending on the actual state of the switch this rule will succeed or fail. Let us now use a dimmer device called *dimmer*.

.. code-block:: guess

   IF dimmer.dimlevel > 10 THEN ...

Again, this rule will succeed or fail depending on the actual dimlevel of the configured dimmer device. These two examples can of course be combined:

.. code-block:: guess

   IF switch.state IS on AND dimmer.dimlevel > 10 THEN ...

As you can also see, the fields (*state* or *dimlevel*) we can use depends on the device we are using inside our rules. A switch does not have a *dimlevel* field but a dimmer does have a *state* field.

.. versionadded:: 8.0 rules based on received codes

Some devices are only used inside rules. Configuring them as explicit devices might sometimes feel a bit bloated. Therefor, pilight allows you to trigger rules based on received codes instead of device updates:

.. deprecated:: 8.1.0

.. code-block:: guess

   IF archtech_switch.state IS on AND archtech_switch.id == 123456 AND arctech_switch.unit == 0 THEN ...

.. versionadded:: 8.1.0

.. code-block:: guess

   IF archtech_switch.state == on AND archtech_switch.id == 123456 AND arctech_switch.unit == 0 THEN ...

In this case, an action will be triggered as soon as an ``archtech_switch`` code is received with a specific state, id, and unitcode. The ``arctech_switch`` doesn't have to be configured as an explicit device for this rule to work.

Functions
---------

In some cases, standard operators limit us in writing our rules. For example, calculating with time is a hideous task considering that hours do not go above 24, minute and seconds do not go above 60, and there are no negative numbers. Other functionality like randomization are also not possible in the standard event operators. This more advanced functionality is added in the form of function. A simple example:

.. code-block:: guess

   IF datetime.hour == RANDOM(21, 23) THEN ...

As we can see in this example we use the RANDOM function to check if the hour is either 21, 22, or 23. This allows us to trigger an action on random hours each day. Actions can also be nested for more advanced logic:

.. code-block:: guess

   IF datetime.hour == RANDOM(RANDOM(21, 22), RANDOM(22, 23)) THEN ...

The output of this RANDOM function is the same as with the previous example, but the idea should be clear.

Actions
-------

Actions are the final goal of our rules. These actions tell pilight what should happen when certain conditions have been met. A rule can contain unlimited number of actions and each action can trigger an unlimited number of devices. First two examples of basic actions triggering a switch called *lamp* and a dimmer called *ambientLight*:

.. code-block:: guess

   IF ... THEN switch DEVICE lamp TO on
   IF ... THEN dim DEVICE ambientLight TO 10

Both actions only trigger a single device. However, if we wanted to trigger both device to just on we can combine them in a single action:

.. code-block:: guess

   IF ... THEN switch DEVICE lamp AND ambientLight TO on

As we can see here, the switch action takes at least the DEVICE and TO parameters. In case of the switch action, several values (as in devices) can be combined by separating them with ANDs. We can also combine dim and switch action would we want to switch the *lamp* to on and dim the *ambientLight* to dimlevel 10 based on the same condition:

.. code-block:: guess

   IF ... THEN switch DEVICE lamp TO on AND dim DEVICE ambientLight TO 10

We can combine an unlimited number of actions like this. Again we see that we use the AND to combine several actions. We can also switch several devices across several actions in a single rule. Let's say we have a relay connected to our television set called television that we want to turn on as well.

.. code-block:: guess

   IF ... THEN switch DEVICE lamp AND television TO on AND dim DEVICE ambientLight TO 10

.. versionadded:: 8.1.0

As described earlier, actions can also be trigger based on a false condition like this:

.. code-block:: guess

   IF ... THEN switch DEVICE lamp AND television TO on ELSE dim DEVICE ambientLight TO 10 END

Using Device Parameters
-----------------------

Device parameters can be used as rule input almost everywhere. Let us look at a few examples to demonstrate this:

.. code-block:: guess

   IF 1 == 1 THEN dim DEVICE dimmer TO dimmerMax.dimlevel FOR dimmerDuration.dimlevel

In this case we use three dimmer devices. One dimmer called dimmer that we actually want to dim, and two dimmers that changes the way this rule behaves. The dimmerMax device tells pilight to what value the dimmer should dim. The dimmerDuration device tells pilight how long it should take to reach that dimlevel. Another example:

.. code-block:: guess

   IF 1 == 1 THEN switch DEVICE lamp1 TO lamp2.state

In this case we want to switch the device lamp1 to the same state as the device lamp2.

Device parameters can also be used in function:

.. code-block:: guess

   IF RANDOM(randomLow.dimlevel, randomHigh.dimlevel) == 10 THEN switch DEVICE lamp1 TO on

In this case we use two dimmers called randomLow and randomHigh to dynamically change the input of the RANDOM function used in this rule. A comprehensive and advanced example:

.. code-block:: guess

   IF sunriseset.sunset == DATE_FORMAT(DATE_ADD(datetime, +1 HOUR), \"%Y-%m-%d %H:%M:%S\", %H.%M) THEN switch DEVICE lamp1 TO on

.. versionchanged:: 8.1.0

.. code-block:: guess

   IF sunriseset.sunset == DATE_FORMAT(DATE_ADD(datetime, '+1 HOUR'), '%Y-%m-%d %H:%M:%S', %H.%M) THEN switch DEVICE lamp1 TO on