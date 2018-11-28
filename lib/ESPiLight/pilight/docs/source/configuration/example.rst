Example
=======

.. warning::

   Make sure pilight is not running before editing your configuration or else all changes will be lost.

.. code-block:: json
   :linenos:

   {
     "devices": {
       "tv": {
         "protocol": ["relay"],
         "id": [{
           "gpio": 3
         }],
         "state": "off",
         "default-state": "off"
       }
     },
     "rules": {
       "lightswitch": {
         "rule": "IF tv.state IS on THEN switch DEVICE tv TO off",
         "active": 1
       }
     },
     "gui": {
       "tv": {
         "name": "Television",
         "group": ["Living"],
         "media": ["all"],
         "readonly": 0
       }
     },
     "settings": {
       "log-level": 6,
       "pid-file": "/var/run/pilight.pid",
       "log-file": "/var/log/pilight.log",
       "webserver-enable": 1,
       "webserver-http-port": 5001,
       "webserver-cache": 1,
       "webserver-root": "/usr/local/share/pilight"
     },
     "hardware": {
       "433gpio": {
       "sender": 0,
       "receiver": 1
       }
     },
     "registry": {
       "pilight": {
         "version": {
           "current": "6.0"
         }
       }
     }
   }