.. |yes| image:: ../../images/yes.png
.. |no| image:: ../../images/no.png

.. role:: underline
   :class: underline

XBMC / Kodi
===========

.. rubric:: Description

Execute rules based on XBMC / KODI based events

.. rubric:: Devices

.. code-block:: json
   :linenos:

   {
     "xbmcControls": {
       "protocol": [ "xbmc" ],
       "id": [{
         "server": "127.0.0.1",
	       "port": 9090
       }],
       "action": "home",
       "media": "none"
     },
     "speakers": {
       "protocol": [ "kaku_switch" ],
       "id": [{
         "id": 123456,
         "unit": 4
       }],
       "state": "off"
     }
   }

.. rubric:: Rule

.. code-block:: json
   :linenos:

   {
     "speakers-on": {
       "rule": "IF xbmcControls.action IS play THEN switch DEVICE speakers TO on",
       "active": 1
     },
     "speakers-off": {
       "rule": "IF xbmcControls.action IS pause OR xbmcControls.action IS home THEN switch DEVICE speakers TO off AFTER 3 MINUTES",
       "active": 1
     }
   }

.. versionchanged:: 8.1.0

.. code-block:: json
   :linenos:

   {
     "speakers-on": {
       "rule": "IF xbmcControls.action == play THEN switch DEVICE speakers TO on",
       "active": 1
     },
     "speakers-off": {
       "rule": "IF xbmcControls.action == pause OR xbmcControls.action == home THEN switch DEVICE speakers TO off AFTER '3 MINUTES'",
       "active": 1
     }
   }
