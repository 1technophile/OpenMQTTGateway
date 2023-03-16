/* 
 OpenMQTTGateway - ESP8266 or Arduino program for home automation 

 Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal and a MQTT broker 
 Send and receiving command by MQTT
 
 This files enables to set your parameter for the DHT11/22 sensor
 
 Copyright: (c)Florian ROBERT
 
 This file is part of OpenMQTTGateway.
 
 OpenMQTTGateway is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenMQTTGateway is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef config_WebContent_h
#define config_WebContent_h

/*------------------- ----------------------*/

const char header_html[] = "<!DOCTYPE html> <html lang=\"en\" class= \" \"><head> <meta charset='utf-8'> <meta name= \"viewport \" content= \"width=device-width,initial-scale=1,user-scalable=no \" /> <title>%s</title> <script> var x = null, lt, to, tp, pc = ''; function eb(s) { return document.getElementById(s); } function qs(s) { return document.querySelector(s); } function sp(i) { eb(i).type = (eb(i).type === 'text' ? 'password' : 'text'); } function wl(f) { window.addEventListener('load', f); }";

const char slash_script[] = "var ft; function la(p) {a = p || '';clearTimeout(ft);clearTimeout(lt);if (x != null) { x.abort()}x = new XMLHttpRequest();x.onreadystatechange = function() { if (x.readyState == 4 && x.status == 200) {var s = x.responseText.replace(/{t}/g,\"<table style='width:100%'> \").replace(/{s}/g,\"<tr><th> \").replace(/{m}/g,\"</th><td style='width:20px;white-space:nowrap'> \").replace(/{e}/g,\"</td></tr> \");eb('l1').innerHTML = s;clearTimeout(ft);clearTimeout(lt);lt = setTimeout(la, 2345); }};x.open('GET', '.?m=1' + a, true);x.send();ft = setTimeout(la, 20000); } function lc(v, i, p) {if (eb('s')) { if (v == 'h' || v == 'd') {var sl = eb('sl4').value;eb('s').style.background = 'linear-gradient(to right,rgb(' + sl + '%,' + sl + '%,' + sl + '%),hsl(' + eb('sl2').value + ',100%%,50%%))'; }}la('&' + v + i + '=' + p); } wl(la);";

const char information_script[] = "function i() { var s, o = \"<table style='width:100%%'><tr><th>%s</td></tr></table>\"; s = o.replace(/}1/g, \"</td></tr><tr><th>\").replace(/}2/g, \"</th><td>\"); eb('i').innerHTML = s; } wl(i);";

const char script[] = "function jd() { var t = 0, i = document.querySelectorAll('input,button,textarea,select'); while (i.length >= t) { if (i[t]) { i[t]['name'] = (i[t].hasAttribute('id') && (!i[t].hasAttribute('name'))) ? i[t]['id'] : i[t]['name']; } t++; } } wl(jd); </script>";

const char style[] = "<style> div, fieldset, input, select { padding: 5px; font-size: 1em; } fieldset { background: #4f4f4f; } p { margin: 0.5em 0; } input { width: 100%; box-sizing: border-box; -webkit-box-sizing: border-box; -moz-box-sizing: border-box; background: #dddddd; color: #000000; } input[type=checkbox], input[type=radio] { width: 1em; margin-right: 6px; vertical-align: -1px; } input[type=range] { width: 99%; } select { width: 100%; background: #dddddd; color: #000000; } textarea { resize: vertical; width: 98%; height: 318px; padding: 5px; overflow: auto; background: #1f1f1f; color: #65c115; } body { text-align: center; font-family: verdana, sans-serif; background: #252525; } td { padding: 0px; } button { border: 0; border-radius: 0.3rem; background: #1fa3ec; color: #faffff; line-height: 2.4rem; font-size: 1.2rem; width: 100%; -webkit-transition-duration: 0.4s; transition-duration: 0.4s; cursor: pointer; } button:hover { background: #0e70a4; } .bred { background: #d43535; } .bred:hover { background: #931f1f; } .bgrn { background: #47c266; } .bgrn:hover { background: #5aaf6f; } a { color: #1fa3ec; text-decoration: none; } .p { float: left; text-align: left; } .q { float: right; text-align: right; } .r { border-radius: 0.3em; padding: 2px; margin: 6px 2px; } </style></head>";

const char slash_body[] = "<body> <div style='text-align:left;display:inline-block;color:#eaeaea;min-width:340px;'> <div style='text-align:center;color:#eaeaea;'> <noscript> To use, please enable JavaScript <br> </noscript> <h3>%s</h3><h2>%s</h2> </div> <div style='padding:0;' id='l1' name='l1'></div> <div id=but3d style='display: block;'></div> <p> <form id=but3 style='display: block;' action='cn' method='get'> <button>Configuration</button> </form> </p> <p> <form id=but4 style='display: block;' action='in' method='get'> <button>Information</button> </form> </p> <p> <form id=but5 style='display: block;' action='up' method='get'> <button>Firmware Upgrade</button> </form> </p> <p> <form id=but14 style='display: block;' action='cs' method='get'> <button>Console</button> </form> </p> <p> <form id=but15 style='display: block;' action='webserial' method='get'> <button>WebSerial</button> </form> </p> <p> <form id=but0 style='display: block;' action='.' method='get' onsubmit='return confirm(\"Confirm Restart\");'> <button name='rst' class='button bred'>Restart</button> </form> </p>";

const char config_body[] = "<body> <div style='text-align:left;display:inline-block;color:#eaeaea;min-width:340px;'> <div style='text-align:center;color:#eaeaea;'> <noscript> To use please enable JavaScript <br> </noscript> <h3>%s</h3><h2>%s</h2> </div> <p> <form id=but7 style='display: block;' action='md' method='get'> <button>Configure Module</button> </form> </p> <p> <form id=but8 style='display: block;' action='wi' method='get'> <button>Configure WiFi</button> </form> </p> <p> <form action='mq' method='get'> <button>Configure MQTT</button> </form> </p> <p> <form action='cl' method='get'><button>Configure Cloud</button> </form> </p> <p> <form action='tm' method='get'> <button>Configure Timer</button> </form> </p> <p> <form id=but9 style='display: block;' action='lg' method='get'> <button>Configure Logging</button> </form> </p> <p> <form id=but10 style='display: block;' action='co' method='get'> <button>Configure Other</button> </form> </p> <p> <form id=but11 style='display: block;' action='tp' method='get'> <button>Configure Template</button> </form> </p> <div id=but1d style='display: block;'></div> <p> <form id=but1 style='display: block;' action='rt' method='get' onsubmit='return confirm(\"Confirm Reset Configuration\");'> <button name='non' class='button bred'>Reset Configuration</button> </form> </p> <p> <form id=but12 style='display: block;' action='dl' method='get'> <button>Backup Configuration</button> </form> </p> <p> <form id=but13 style='display: block;' action='rs' method='get'> <button>Restore Configuration</button> </form> </p> <div id=but2d style='display: block;'></div> <p> <form id=but2 style='display: block;' action='.' method='get'> <button>Main Menu</button> </form> </p>";

const char reset_body[] = "<body><div style='text-align:left;display:inline-block;color:#eaeaea;min-width:340px;'><div style='text-align:center;color:#eaeaea;'><noscript>To use please enable JavaScript<br></noscript><h3>%s</h3><h2>%s</h2></div><div style='text-align:center;'>Configuration reset</div><br><div style='text-align:center;'>Device will restart in a few seconds</div><br><div id=but2d style=\"display: block;\"></div><p><form id=but2 style=\"display: block;\" action='.' method='get'><button>Main Menu</button></form></p>";

const char cloud_body[] = "<body><div style='text-align:left;display:inline-block;color:#eaeaea;min-width:340px;'><div style='text-align:center;color:#eaeaea;'><noscript>To use Tasmota, please enable JavaScript<br></noscript><h3>%s</h3><h2>%s</h2></div><fieldset><legend><b>&nbsp;Cloud Configuration&nbsp;</b></legend><form method='get' action='cl'><p><label><input id='cl-en' type='checkbox' %s><b>Enable Cloud Connection</b></label></p><br><p> <label><input id='cl-lk' type='checkbox' disabled><b>Cloud Account%s Linked</b></label></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset><p><form action='%s' method='get'><input type='hidden' name='macAddress' value='%s' /><input type='hidden' name='redirect_uri' value='%s' /><input type='hidden' name='gateway_name' value='%s' /><button>Link Cloud Account</button></form></p><div id=but3d style=\"display: block;\"></div><p><form id=but3 style=\"display: block;\" action='cn' method='get'><button>Configuration</button></form></p>";

const char token_body[] = "<body><div style='text-align:left;display:inline-block;color:#eaeaea;min-width:340px;'><div style='text-align:center;color:#eaeaea;'><noscript>To use please enable JavaScript<br></noscript><h3>%s</h3><h2>%s</h2></div><div style='text-align:center;'>Link Cloud Account</div><br><div style='text-align:center;'>Cloud was successfully linked</div><br><div id=but2d style=\"display: block;\"></div><p><form id=but2 style=\"display: block;\" action='.' method='get'><button>Main Menu</button></form></p>";

const char information_body[] = "<body><div style='text-align:left;display:inline-block;color:#eaeaea;min-width:340px;'><div style='text-align:center;color:#eaeaea;'><noscript>To use please enable JavaScript<br></noscript><h3>%s</h3><h2>%s</h2></div><style>td {padding: 0px 5px;}</style><div id='i' name='i'></div><div id=but2d style=\"display: block;\"></div><p><form id=but2 style=\"display: block;\" action='.' method='get'><button>Main Menu</button></form></p>";

const char footer[] = "<div style='text-align:right;font-size:11px;'> <hr /> <a href='https://community.openmqttgateway.com' target='_blank' style='color:#aaa;'>%s</a> </div> </div></body></html>";

#endif