Debugging
=========

- `Introduction`_
- `Prerequisites`_
- `How to recognize a Bug?`_
- `How to Debug?`_

Introduction
------------

pilight is a product developed by a growing community. Due to its Open Source character, developers around the world can contribute code. Although all developers try to write bug free code, it is not always possible for the developers of pilight to check for every possible mistake that can be made. Especially, because not everyone has every hardware available for testing.

Prerequisites
-------------

If you found this page, you probably encountered a bug in the pilight code or noticed that things are not working as they should be. Before you continue reading, consider the following.

First, pilight offers code in several stages of stability:

#. Stable apt repository and GitHub master branch
   This code has been tested to work without bugs for various weeks by the developer and a large community. If bugs are found in this version of pilight, the problem has probably already been fixed in the developmental code.

#. Nightly apt repository and GitHub development branch
   This code always contains the very latest features and fixes. It is updated several times a week. However, this also means that not all bugs are already encountered when a user starts to use it.

Second, this page is made for those users using the developmental GitHub branch. This tutorial assumes the following:

#. You are aware of the distinction between bugs and changes in the pilight functionality.
#. You have read the GitHub commit messages to be sure that what you have encountered is indeed a bug and not a changed functionality.
#. You have enough experience working with your operating system.
#. You have the patience to help finding the problem.
#. You have some knowledge about programming (and the C language).
#. You know when to recognize a clear code bug.
#. You are able to manually compile pilight so debugging is possible. The automatically compiled versions do not support debugging as described here.

How to Recognize a Bug?
-----------------------

The two most encountered bugs are the memory issues or the segmentation faults. A memory bug can look like this:

.. code-block:: console

   * glibc detected ./pilight-daemon: double free or corruption (!logfile): 0x0000000000c6ed50 **

This message means in this case that the variable logfile was freed twice.

A segmentation fault looks like this:

.. code-block:: console

   pi@raspberrypi ~/pilight $ sudo service pilight start
   [....] Starting : pilightSegmentation fault
   failed!

This message means there has been a general error in which pilight so it cannot be started properly.

How to Debug?
-------------

To debug C code, the GNU Debugger is used, also called gdb. This program will run next to pilight without any performance issue, but when pilight crashes, it allows us to track the specific line where the error occurred. So first of all, make sure you have gdb installed. This is how you start gdb together with pilight:

.. code-block:: console

   pi@pilight:~> sudo su
   root@pilight:~# gdb --args pilight-daemon -D
   GNU gdb (GDB) 7.4.1-debian
   Copyright (C) 2012 Free Software Foundation, Inc.
   License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
   This is free software: you are free to change and redistribute it.
   There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
   and "show warranty" for details.
   This GDB was configured as "arm-linux-gnueabihf".
   For bug reporting instructions, please see:
   <http://www.gnu.org/software/gdb/bugs/>...
   Reading symbols from /usr/local/sbin/pilight-daemon...done.
   (gdb) run
   Starting program: /usr/local/sbin/pilight-daemon -D
   [Thread debugging using libthread_db enabled]
   Using host libthread_db library "/lib/arm-linux-gnueabihf/libthread_db.so.1".
   [Jan 18 16:23:33:657363] pilight-daemon: DEBUG: ssdp sent search
   [Jan 18 16:23:33:701618] pilight-daemon: NOTICE: no pilight daemon found, daemonizing
   [Jan 18 16:23:33:718878] pilight-daemon: DEBUG: -- start parsed config file --

Now pilight will start as it you would have started it normally. Just let it run like this until it crashes or try to invoke the crash. If it crashes, you will see something like this:

.. code-block:: console

   *** glibc detected *** /usr/local/sbin/pilight-daemon: double free or corruption (fasttop): 0x000369d8 ***
   [New Thread 0xb228c470 (LWP 26235)]
   Program received signal SIGABRT, Aborted.
   [Switching to Thread 0xb4d7d470 (LWP 26230)]
   0xb6dddbfc in __GI_raise (sig=6) at ../nptl/sysdeps/unix/sysv/linux/raise.c:67
   67      ../nptl/sysdeps/unix/sysv/linux/raise.c: No such file or directory.
   (gdb)

Again, you see the **(gdb)** prompt that allows you to type new commands.

The first step is to get a backtrace of all functions called when this crash occurred:

.. code-block:: console

   (gdb) backtrace
   #0  0xb6dddbfc in __GI_raise (sig=6)
       at ../nptl/sysdeps/unix/sysv/linux/raise.c:67
   #1  0xb6de197c in __GI_abort () at abort.c:92
   #2  0xb6e15cf8 in __libc_message (do_abort=2,
       fmt=0xb6ec78d0 "*** glibc detected *** %s: %s: 0x%s ***\n")
       at ../sysdeps/unix/sysv/linux/libc_fatal.c:189
   #3  0xb6e20584 in malloc_printerr (action=3,
       str=0xb6ec7a6c "double free or corruption (fasttop)", ptr=<optimized out>)
       at malloc.c:6283
   #4  0xb6e24e00 in __GI___libc_free (mem=<optimized out>) at malloc.c:3738
   #5  0x0000b5d8 in broadcast (param=<optimized out>) at daemon.c:260
   #6  0xb6ee6bfc in start_thread (arg=0xb4d7d470) at pthread_create.c:306
   #7  0xb6e7b758 in ?? ()
       at ../ports/sysdeps/unix/sysv/linux/arm/nptl/../clone.S:116
      from /lib/arm-linux-gnueabihf/libc.so.6
   #8  0xb6e7b758 in ?? ()
       at ../ports/sysdeps/unix/sysv/linux/arm/nptl/../clone.S:116
      from /lib/arm-linux-gnueabihf/libc.so.6
   Backtrace stopped: previous frame identical to this frame (corrupt stack?)
   (gdb)

Copy this output so it can be communicated on the forum.

The second step is to look for the exact line causing this crash. This can be done by zooming into each part of each function related to this bug. We start at the end and work our way to the beginning. To zoom into a function the command **frame** # is used in which the # is replaced by one of the numbers you see in the backtrace. As said earlier, we will start at the end:

.. code-block:: console

   (gdb) frame 8
   #8  0xb6e7b758 in ?? ()
       at ../ports/sysdeps/unix/sysv/linux/arm/nptl/../clone.S:116
      from /lib/arm-linux-gnueabihf/libc.so.6
   116     ../ports/sysdeps/unix/sysv/linux/arm/nptl/../clone.S: No such file or directory.

This doesn't tell us anything useful. So we continue to the previous frame.

.. code-block:: console

   (gdb) frame 7
   #7  0xb6e7b758 in ?? ()
       at ../ports/sysdeps/unix/sysv/linux/arm/nptl/../clone.S:116
      from /lib/arm-linux-gnueabihf/libc.so.6
   116     ../ports/sysdeps/unix/sysv/linux/arm/nptl/../clone.S: No such file or directory.

Still no useful information here.

.. code-block:: console

   (gdb) frame 6
   #6  0xb6ee6bfc in start_thread (arg=0xb4d7d470) at pthread_create.c:306
   306     pthread_create.c: No such file or directory.

Nope.

.. code-block:: console

   (gdb) frame 5
   #5  0x0000b5d8 in broadcast (param=<optimized out>) at daemon.c:260
   260                                     json_delete(jret);

Yes, finally something related to pilight. As you can see here, the conflicting line can be found in daemon.c in the function broadcast at line 260. The actual command that leaded to the crash was json_delete. Although we now know that this was the line causing the crash, the actual bug could lay elsewhere. Therefore we continue until the last pilight related frame.

No useful information in frame 4. So, frame 5 was the only interesting frame here. Make sure to copy the output of the informative frames so it can be used in a forum post later on.

To exit:

.. code-block:: console

   (gdb) q

As you might have seen, this way of debugging makes it a lot easier to track bugging code. gdb will tell you the exact line on which the bug occurred to in can be easier tracked down and fixed.