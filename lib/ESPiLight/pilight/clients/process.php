#!/usr/bin/env php
<?php
/*
	Copyright (C) 2013 CurlyMo

	This file is part of pilight.

    pilight is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

    pilight is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pilight. If not, see	<http://www.gnu.org/licenses/>
*/

$sMsg = "M-SEARCH * HTTP/1.1\r\n";
$sMsg .= "Host:239.255.255.250:1900\r\n";
$sMsg .= "ST:urn:schemas-upnp-org:service:pilight:1\r\n";
$sMsg .= "Man:\"ssdp:discover\"\r\n";
$sMsg .= "MX:3\r\n\r\n";

$sBuffer = NULL;
$sTmp = NULL;
$rSocket = socket_create(AF_INET, SOCK_DGRAM, 0);
socket_set_option($rSocket, SOL_SOCKET, SO_RCVTIMEO, array('sec'=>0, 'usec'=>10000));
$send_ret = socket_sendto($rSocket, $sMsg, 1024, 0, '239.255.255.250', 1900);
$aHosts = Array();
$x = 0;
while(@socket_recvfrom($rSocket, $sBuffer, 1024, MSG_WAITALL, $sTmp, $sTmp)) {
	if(strpos($sBuffer, 'pilight') !== false) {
		$aLines = explode("\r\n", $sBuffer); 
		foreach($aLines as $sLine) {
			if(sscanf($sLine, "Location:%d.%d.%d.%d:%d\r\n", $ip1, $ip2, $ip3, $ip4, $port) > 0) {
				$ip = $ip1.'.'.$ip2.'.'.$ip3.'.'.$ip4;
				$aHosts[$x]['ip'] = $ip;
				$aHosts[$x]['port'] = $port;
				$x++;
			}
		}
	}
}
socket_close($rSocket);
$sLine = '';
if(count($aHosts) > 0) {
	$fp = fsockopen($aHosts[0]['ip'], $aHosts[0]['port'], $errno, $errdesc) or die ("Couldn't connect to server");
	fputs($fp, '{"action":"identify","options":{"receiver":1}}');
	$sLine = fgets($fp, 1024);
	if($sLine == "{\"status\":\"success\"}\n") {
		$iLen = 0;
		$sLine = '';
		while(true) {
			$sLine .= fgets($fp, 1024);
			$iLen = strlen($sLine);
			if($iLen > 2 && ord($sLine[$iLen-1]) === 10 && ord($sLine[$iLen-2]) === 10) {
				$sLine = substr($sLine, 0, -2);
				echo $sLine."\n";
				$sLine = '';
			}
		}
	}
} else {
	die("no pilight ssdp connections found\n");
}
?>