#!/usr/bin/env python
#
# Copyright (C) 2017 pilino1234
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

"""Example Python 3 client for pilight daemon socket API

Runs a SSDP discovery for pilight daemons on the network, and connects to it.
Identifies as a "receiver" type client, and prints received messages until
terminated.

"""

import socket
import struct
import re


def discover(service, retries=1):
    """SSDP discovery function

    :param service: The name of the SSDP service to run discovery for
    :type service: str
    :param retries: The number of discovery retries. Defaults to 1.
    :type retries: int

    """

    group = ("239.255.255.250", 1900)
    discover_message = "\r\n".join([
        'M-SEARCH * HTTP/1.1',
        'HOST: {0}:{1}'.format(*group),
        'MAN: "ssdp:discover"',
        'ST: {st}', 'MX: 3', '', '']).format(st=service)

    ssdp_responses = {}

    for i in range(retries):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM,
                             socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO,
                        struct.pack('LL', 0, 10000))
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
        sock.sendto(bytes(discover_message, "utf-8"), group)

        while True:
            try:
                ssdp_responses[i] = sock.recv(1024).decode("utf-8")
                break
            except socket.timeout:
                break
            except IOError:
                print("no pilight ssdp connections found")
                break

    sock.close()
    return ssdp_responses


if __name__ == "__main__":

    RESPONSES = discover("urn:schemas-upnp-org:service:pilight:1")

    if len(RESPONSES) > 0:
        for resp in RESPONSES:  # try all responses until one works
            locationsrc = re.search("Location:([0-9.]+):(.*)",
                                    str(RESPONSES[resp]), re.IGNORECASE)

            if locationsrc:
                location = locationsrc.group(1)
                port = locationsrc.group(2)

                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    try:
                        s.connect((location, int(port)))
                    except ConnectionRefusedError:
                        # Connection was refused, try next SSDP response
                        print("connection refused")
                        continue

                    s.send(bytes('{"action": "identify", \
                                   "options": {"receiver": 1}}', "utf-8"))
                    response = ""
                    while True:
                        line = s.recv(1024).decode("utf-8")
                        response += line
                        if "\n\n" in line[-2:]:
                            response = response[:-2]
                            break

                    if response == '{"status":"success"}':
                        message = ""
                        while True:
                            line = s.recv(1024).decode("utf-8")
                            message += line
                            if "\n\n" in line[-2:]:
                                message = message[:-2]
                                for f in iter(message.splitlines()):
                                    print(f)
                                message = ""
                    elif response == '{"status":"failure"}':
                        # connection failed, try next SSDP response
                        print("connection failed")
                        continue
