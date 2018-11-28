:tocdepth: 5

Welcome to pilight's documentation!
===================================

.. raw:: latex

   \newpage

Overview
--------

pilight is a free, open source, full fledged domotica solution that runs on a Raspberry Pi, Hummingboard, and BananaPi, but also on \*BSD, various Linux distributions (tested on Arch, Ubuntu, and Debian), Windows, and OpenWRT routers. pilight works with a great deal of devices and is frequency independent. Therefore, due to its modular set-up, it can control devices working at 315 MHz, 433 MHz, and 868 MHz, but only 433.92 MHz is currently supported. Support for other frequencies and/or devices is dependent on community input, because we as developers do not own them all.

.. raw:: latex

   \newpage

Features
--------
The main features of pilight are:

* `Open Source`_
* `Open Hardware`_
* `Free`_
* `Fully Modular`_
* `High Quality Code`_
* `Integrated Webserver and webGUI`_
* `Light-Weight`_

.. _Open Source:
.. rubric:: Open Source

The pilight source code can be found on the internet at https://www.github.com/pilight/. pilight is largely dependent on community input so if you have any suggestions, improvements, or fixes, feel free to provide them.

.. _Open Hardware:
.. rubric:: Open Hardware

The source and schematics of all pilight hardware is open and free for everyone. This means that you can easily build the hardware pilight offers yourself. However, by buying the hardware from pilight, you also support the project.

.. _Free:
.. rubric:: Free

pilight is free as in free speech. It is licensed under the GPLv3 license. If you have any plans with our source code, please make it clear that you are not affiliated to pilight in any way. We do not want to end up in a situation were we have to give support for something we did not approve.

.. _Fully Modular:
.. rubric:: Fully Modular

pilight is modular in about every inch. Adding new protocols is very easy just like adding new hardware modules. The API allows users to develop new applications that interact with pilight. The community already developed phone apps, desktop apps, and various plug-ins for various other projects.

.. _High Quality Code:
.. rubric:: High quality code

Almost everything pilight does is also developed by pilight from the ground up. This means that the developers of pilight know what is going on in every part of the pilight code. Whenever additional functionality is needed, external libraries are carefully selected for their own code quality. As an example, pilight does not use OpenSSL but polarSSL (currently called mbed TLS). OpenSSL has a bad reputation in terms of code quality while polarSSL is far more robust. pilight developers also constantly audit their own code by using tools like valgrind or gdb. Memory leaks or other common issues are therefore 99% nonexisting in pilight. The pilight code quality also allows it to be ported easily to any platform as was proven with the Windows and OpenWRT ports.

.. _Integrated Webserver and webGUI:
.. rubric:: Integrated webserver and webGUI

pilight has an integrated webserver and serves an intergrated webGUI. This allows users to immediately start using pilight in all its capacity. The webGUI makes it easy to control devices or to see their states. Those who do not want to install the phone apps can just use their regular browser to access pilight.

.. _Light-Weight:
.. rubric:: Light-Weight

pilight is written in C. This means that it is very fast and does not use much resources. All plug-ins are compiled into pilight so the overhead of the plug-ins is also kept to a minimum. This is also one of the reasons pilight can run on low-power consumer products like OpenWRT routers. All pilight hardware such as the ATTiny filter and the Arduino Nano interface are also written in pure C. This again allows us to have control over every inch of code and to prevent any overhead from tools like the Arduino programming libraries.

.. raw:: latex

   \newpage

Sitemap
-------

.. toctree::
   :maxdepth: 4
   :titlesonly:

   changelog
   installation
   configuration/index
   protocols/index
   eventing/index
   adhoc_network
   programs/index
   electronics/index
   faq
   development/index
   about_manual

