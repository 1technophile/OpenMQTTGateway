.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

Program
=======

+------------------+-------------+
| **Feature**      | **Support** |
+------------------+-------------+
| Sending          | |yes|       |
+------------------+-------------+
| Receiving        | |yes|       |
+------------------+-------------+
| Config           | |yes|       |
+------------------+-------------+

.. rubric:: Supported Brands

*None*

.. rubric:: Sender Arguments

.. code-block:: console

   -t --running             start the program
   -f --stopped             stop the program
   -n --name=name           name of the program

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "XBMC": {
         "protocol": [ "program" ],
         "id": [{
           "name": "xbmc"
         }],
         "program": "/usr/local/lib/xbmc/xbmc.bin",
         "arguments": "",
         "stop-command": "service xbmc stop",
         "start-command": "service xbmc start",
         "state": "running",
         "pid": 11642
       }
     },
     "gui": {
       "xbmc": {
         "name": "XBMC",
         "group": [ "Programs" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| name             | *any value*     |
+------------------+-----------------+
| arguments        | *any value*     |
+------------------+-----------------+
| stop-command     | *any value*     |
+------------------+-----------------+
| start-command    | *any value*     |
+------------------+-----------------+
| state            | | starting      |
|                  | | running       |
|                  | | stopped       |
+------------------+-----------------+
| pid              | 0 - 9           |
+------------------+-----------------+

.. rubric:: Optional Settings

:underline:`Device Settings`

+------------------+-------------+------------+-------------------------------------------------+
| **Setting**      | **Default** | **Format** | **Description**                                 |
+------------------+-------------+------------+-------------------------------------------------+
| poll-interval    | 1           | >= 1       | How ofter do we want to check the program state |
+------------------+-------------+------------+-------------------------------------------------+

:underline:`GUI Settings`

+------------------+-------------+------------+-----------------------------------------------+
| **Setting**      | **Default** | **Format** | **Description**                               |
+------------------+-------------+------------+-----------------------------------------------+
| readonly         | 0           | 1 or 0     | Disable controlling this device from the GUIs |
+------------------+-------------+------------+-----------------------------------------------+
| confirm          | 0           | 1 or 0     | Ask for confirmation when switching device    |
+------------------+-------------+------------+-----------------------------------------------+

.. rubric:: Comment

The program protocol takes two important arguments

- The program
- The arguments

You can run ``ps aux`` to find out what these are. An example:

.. code-block:: console

   USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
   root     11622  0.0  2.3  20216  8860 tty4     Ssl+ 20:04   0:01 /usr/local/sbin/splash-daemon
   root     11636  0.0  0.4   4096  1540 ?        Ss   20:04   0:00 sudo -u xbian /usr/local/lib/xbmc/xbmc.bin --standalone -fs --lircdev /run/lirc/lircd
   xbian    11642 22.5 12.6 390836 48280 ?        RNl  20:04  15:09 /usr/local/lib/xbmc/xbmc.bin --standalone -fs --lircdev /run/lirc/lircd
   xbian    11734  0.0  0.2   2692  1092 ?        S    20:04   0:00 /bin/bash
   root     11868  0.0  1.4   9732  5432 ?        Ss   20:04   0:00 python /usr/local/sbin/upstart-xbmc-bridge.py
   root     11908  0.0  0.0      0     0 ?        S    20:06   0:00 [btrfs-delalloc-]
   root     13078  0.0  0.0      0     0 ?        S    21:05   0:00 [kworker/u2:0]
   root     13085  0.0  0.0      0     0 ?        S    21:08   0:00 [btrfs-worker-2]
   root     13166  0.0  0.0      0     0 ?        S    21:10   0:00 [kworker/u2:1]
   root     13186  0.0  0.2   4424  1116 pts/2    R+   21:11   0:00 ps aux

In thise case, i want to control XBMC. The xbian entry with the PID 11642 is what i want to use. The first word is the program name. In this case ``/usr/local/lib/xbmc/xbmc.bin``. Everything that comes after are the arguments ``–standalone -fs –lircdev /run/lirc/lircd``. pilight will now check for a program that matches these exact parameters. So the protocol looks for processes matching those lines and when a process is found, it updates the pid value accordingly. (The pid is just informative.)

If you omit the arguments value, all matching programs will be processes with or without matching arguments. If there are more than once matches, pilight will use the first.
