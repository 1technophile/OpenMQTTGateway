.. |yes| image:: ../../../images/yes.png
.. |no| image:: ../../../images/no.png

.. role:: underline
   :class: underline

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

+----------------------+--------------+
| **Brand**            | **Protocol** |
+----------------------+--------------+
| KlikAanKlikUit       | kaku_dimmer  |
+----------------------+--------------+
| Nexa WMR-252         | kaku_dimmer  |
+----------------------+--------------+
| Intertechno ITDM-250 | kaku_dimmer  |
+----------------------+--------------+
| Intertechno ITDM-150 | kaku_dimmer  |
+----------------------+--------------+
| Intertechno ITL-210  | kaku_dimmer  |
+----------------------+--------------+
| Intertechno ITL-250  | kaku_dimmer  |
+----------------------+--------------+
| Intertechno ITLR-300 | kaku_dimmer  |
+----------------------+--------------+

.. rubric:: Sender Arguments

.. code-block:: console
   :linenos:

   -u --unit=unit             control a device with this unit code
   -i --id=id                 control a device with this id
   -a --all                   send command to all devices with this id
   -d --dimlevel=dimlevel     send a specific dimlevel
   -l --learn                 send multiple streams so dimmer can learn

.. rubric:: Config

.. code-block:: json
   :linenos:

   {
     "devices": {
       "dimmer": {
         "protocol": [ "kaku_dimmer" ],
         "id": [{
           "id": 100,
           "unit": 0
         }],
         "state": "off",
         "dimlevel": 15
       }
     },
     "gui": {
       "dimmer": {
         "name": "Dimmer",
         "group": [ "Living" ],
         "media": [ "all" ]
       }
     }
   }

+------------------+-----------------+
| **Option**       | **Value**       |
+------------------+-----------------+
| id               | 0 - 67108863    |
+------------------+-----------------+
| code             | 0 - 31          |
+------------------+-----------------+
| dimlevel         | 0 - 15          |
+------------------+-----------------+
| state            | on / off        |
+------------------+-----------------+

.. rubric:: Optional Settings

:underline:`Device Settings`

+--------------------+-------------+------------+-------------------------------------------------+
| **Setting**        | **Default** | **Format** | **Description**                                 |
+--------------------+-------------+------------+-------------------------------------------------+
| dimlevel-minimum   | 0           | number     | Minimum dimlevel                                |
+--------------------+-------------+------------+-------------------------------------------------+
| dimlevel-maximum   | 15          | number     | Maximum dimlevel                                |
+--------------------+-------------+------------+-------------------------------------------------+
| all                | 0           | 1 or 0     | | If specified this will trigger the "group"    |
|                    |             |            | | function of the advanced remotes and trigger  |
|                    |             |            | | all registered devices for the given unitcode |
+--------------------+-------------+------------+-------------------------------------------------+

:underline:`GUI Settings`

+----------------------+-------------+------------+-----------------------------------------------------------+
| **Setting**          | **Default** | **Format** | **Description**                                           |
+----------------------+-------------+------------+-----------------------------------------------------------+
| readonly             | 1           | 1 or 0     | Disable controlling this device from the GUIs             |
+----------------------+-------------+------------+-----------------------------------------------------------+
| confirm              | 1           | 1 or 0     | Ask for confirmation when switching device                |
+----------------------+-------------+------------+-----------------------------------------------------------+

.. rubric:: Notes

If you want your switch to learn new codes, you can use the following command:

.. code-block:: console

   pilight-send -p kaku_dimmer -i 1 -u 1 -t -l

If you want your switch to remove a learned code, you can use the following command:

.. code-block:: console

   pilight-send -p kaku_dimmer -i 1 -u 1 -f -l

.. rubric:: Protocol

This protocol sends 148 pulses like this

.. code-block:: console

   286 2825 286 201 289 1337 287 209 283 1351 287 204 289 1339 288 207 288 1341 289 207 281 1343 284 205 292 1346 282 212 283 1348 282 213 279 1352 282 211 281 1349 282 210 283 1347 284 211 288 1348 281 211 285 1353 278 213 280 1351 280 232 282 1356 279 213 285 1351 276 215 285 1348 277 216 278 1359 278 216 279 1353 272 214 283 1358 276 216 276 1351 278 214 284 1357 275 217 276 1353 270 217 277 1353 272 220 277 1351 275 220 272 1356 275 1353 273 224 277 236 282 1355 272 1353 273 233 273 222 268 1358 270 219 277 1361 274 218 280 1358 272 1355 271 243 273 222 268 1358 270 219 277 1361 274 218 280 1358 272 1355 271 243 251 11302

The first 2 pulses are the ``header`` and the last 2 pulses are the ``footer``. These are meant to identify the pulses as genuine. We don't use them for further processing. The next step is to transform this output into 36 groups of 4 pulses (and thereby dropping the ``header`` and ``footer`` pulses).

.. code-block:: console

   286 201 289 1337
   287 209 283 1351
   287 204 289 1339
   288 207 288 1341
   289 207 281 1343
   284 205 292 1346
   282 212 283 1348
   282 213 279 1352
   282 211 281 1349
   282 210 283 1347
   284 211 288 1348
   281 211 285 1353
   278 213 280 1351
   280 232 282 1356
   279 213 285 1351
   276 215 285 1348
   277 216 278 1359
   278 216 279 1353
   272 214 283 1358
   276 216 276 1351
   278 214 284 1357
   275 217 276 1353
   270 217 277 1353
   272 220 277 1351
   275 220 272 1356
   275 1353 273 224
   277 236 282 1353
   272 1353 273 277
   273 222 268 1358
   270 219 277 1361
   274 218 280 1358
   272 1355 271 243
   273 222 268 1358
   270 219 277 1361
   274 218 280 1358
   272 1355 271 243

If we now look at carefully at these groups you can distinguish three types of groups:

- ``272 1355 271 243``
- ``274 218 280 1358``
- ``277 236 282 277``

So the first group is defined by a high 2nd, the second group has a high 4th pulse, and the third group has a low 2nd and a low 4th pulse. This third group is a special one but not much of a use while receiving. However, it must be taken care of when sending arctech dimmer codes. So we take either of these two pulses to define a 0 or a 1. In this case we say a high 2nd pulse means a 1 and a low 2nd pulse means a 0. We then get the following output:

.. code-block:: console

   000000000000000000000000010100010001

Each (group) of numbers has a specific meaning:

- ID: 0 till 25
- All: 26
- Check: 27
- Unit: 28 till 31
- Dimlevel: 32 till 35

.. code-block:: console

   00000000000000000000000001 0 1 0001 0001

- The ``ID`` is defined as a binary number
- The ``All`` special pulse to identify the code was meant for a dimmer
- The ``Check`` defines whether a devices needs to be turned On or Off
- The ``Unit`` is also defined as a binary number
- The ``Dimlevel`` defines the specific dimlevel

So this code represents:

- ID: 1
- All: Single
- Check: Valid
- Unit: 1
- Dimlevel: 1

Another example:

- ID: 123456
- All: All
- Check: Valid
- Unit: 15
- Dimlevel: 8

.. code-block:: console

   000000000111100010010000001011111000