#!/usr/bin/env perl
#
#	Copyright (C) 2013 CurlyMo
#
#	This file is part of pilight.
#
#   pilight is free software: you can redistribute it and/or modify it under the
#	terms of the GNU General Public License as published by the Free Software
#	Foundation, either version 3 of the License, or (at your option) any later
#	version.
#
#   pilight is distributed in the hope that it will be useful, but WITHOUT ANY
#	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
#	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with pilight. If not, see	<http://www.gnu.org/licenses/>
#

use IO::Socket;

socket(sock, PF_INET, SOCK_DGRAM, getprotobyname("udp")) or die "socket:$@";
setsockopt(sock, SOL_SOCKET, SO_BROADCAST, 1) or die "setsockopt:$@";
setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, pack('l!l!', 0, 10000)) or die "setsockopt:$@";
my $dest = sockaddr_in(1900, inet_aton('239.255.255.250'));

$msg = "M-SEARCH * HTTP/1.1\r\n";
$msg .= "Host:239.255.255.250:1900\r\n";
$msg .= "ST:urn:schemas-upnp-org:service:pilight:1\r\n";
$msg .= "Man:\"ssdp:discover\"\r\n";
$msg .= "MX:3\r\n\r\n";

my $ip = '';
my $port = '';
send(sock, $msg, 1024, $dest);
while(recv(sock, $data, 1024, $dest)) {
	if(index($data, "pilight") != -1) {
		foreach(split(/\r\n/, $data)) {
			$_ =~ /Location:([0-9\.]+):([0-9]+)/ || next;
			($ip,$port) = ($1,$2);
		}
	}
}

$socket = new IO::Socket::INET (
    PeerHost  => $ip,
    PeerPort  =>  $port,
    Proto => 'tcp',
) or die "no pilight ssdp connections found\n";

$socket->send('{"action":"identify","options":{"receiver":1}}');
$socket->recv($recv_data, 1024);
my $text = '';
while(1) {
	$text = '';
	while(1) {
		$socket->recv($recv_data, 1024);
		$text .= $recv_data;
		if($recv_data[(length $recv_data)-1] == '\n' || $recv_data[(length $recv_data)] == '\n') {
			$text = substr $text, 0, -1;
			last;
		}
	}
	print $text;
}