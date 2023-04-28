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

// TODO: Create a script to generate these from WebPack

#define body_footer_main_menu   "<div id=but2d style=\"display: block;\"></div><p><form id=but2 style=\"display: block;\" action='.' method='get'><button>Main Menu</button></form></p>"
#define body_footer_config_menu "<div id=but3d style=\"display: block;\"></div><p><form id=but3 style=\"display: block;\" action='cn' method='get'><button>Configuration</button></form></p>"
#define body_header             "<body><div style='text-align:left;display:inline-block;min-width:360px;'><div style='text-align:center;'><noscript> To use, please enable JavaScript <br></noscript><h3>%s</h3><h2>%s</h2></div>"

#if defined(ESP32) && defined(MQTT_HTTPS_FW_UPDATE)
#  define button_upgrade "<p><form id=but5 style='display: block;' action='up' method='get'><button>Firmware Upgrade</button></form></p>"
#else
#  define button_upgrade ""
#endif
// Configuration Menu

#define configure_1 "<p><form action='wi' method='get'><button>Configure WiFi</button></form></p>"
#define configure_2 "<p><form action='mq' method='get'><button>Configure MQTT</button></form></p>"
#if defined(ZgatewayCloud)
#  define configure_3 "<p><form action='cl' method='get'><button>Configure Cloud</button></form></p>"
#else
#  define configure_3
#endif
#define configure_4 "<p><form action='wu' method='get'><button>Configure WebUI</button></form></p>"
#define configure_5 "<p><form action='lo' method='get'><button>Configure Logging</button></form></p>"
#define configure_6
#define configure_7
#define configure_8

/*------------------- ----------------------*/

const char header_html[] = "<!DOCTYPE html><html lang=\"en\" class= \" \"><head><meta charset='utf-8'><meta name= \"viewport \" content= \"width=device-width,initial-scale=1,user-scalable=no \"/><title>%s</title><script> var x = null, lt, to, tp, pc = ''; function eb(s) { return document.getElementById(s); } function qs(s) { return document.querySelector(s); } function sp(i) { eb(i).type = (eb(i).type === 'text' ? 'password' : 'text'); } function wl(f) { window.addEventListener('load', f); }";

const char root_script[] = "var ft; function la(p) {a = p || '';clearTimeout(ft);clearTimeout(lt);if (x != null) { x.abort()}x = new XMLHttpRequest();x.onreadystatechange = function() { if (x.readyState == 4 && x.status == 200) {var s = x.responseText.replace(/{t}/g,\"<table style='width:100%'> \").replace(/{s}/g,\"<tr><th> \").replace(/{m}/g,\"</th><td style='width:20px;white-space:nowrap'> \").replace(/{e}/g,\"</td></tr> \");eb('l1').innerHTML = s;clearTimeout(ft);clearTimeout(lt);lt = setTimeout(la, 2345); }};x.open('GET', '.?m=1' + a, true);x.send();ft = setTimeout(la, 20000); } function lc(v, i, p) {if (eb('s')) { if (v == 'h' || v == 'd') {var sl = eb('sl4').value;eb('s').style.background = 'linear-gradient(to right,rgb(' + sl + '%,' + sl + '%,' + sl + '%),hsl(' + eb('sl2').value + ',100%%,50%%))'; }}la('&' + v + i + '=' + p); } wl(la);";

const char restart_script[] = "setTimeout(function() { location.href = '.'; }, 15000);";

const char information_script[] = "function i() { var s, o = \"<table style='width:100%%'><tr><th>%s</td></tr></table>\"; s = o.replace(/}1/g, \"</td></tr><tr><th>\").replace(/}2/g, \"</th><td>\"); eb('i').innerHTML = s; } wl(i);";

const char console_script[] = "var sn = 0, id = 0, ft, ltm = 2345; function l(p) { var c, o = ''; clearTimeout(lt); clearTimeout(ft); t = eb('t1'); if (p == 1) { c = eb('c1'); o = '&c1=' + encodeURIComponent(c.value); c.value = ''; t.scrollTop = 99999; sn = t.scrollTop; } if (t.scrollTop >= sn) { if (x != null) { x.abort(); } x = new XMLHttpRequest(); x.onreadystatechange = function() { if (x.readyState == 4 && x.status == 200) { var z, d; d = x.responseText.split(/}1/); id = d.shift(); if (d.shift() == 0) { t.value = ''; } z = d.shift(); if (z.length > 0) { t.value += z; } t.scrollTop = 99999; sn = t.scrollTop; clearTimeout(ft); lt = setTimeout(l, ltm); } }; x.open('GET', 'cs?c2=' + id + o, true); x.send(); ft = setTimeout(l, 20000); } else { lt = setTimeout(l, ltm); } return false; } wl(l); var hc = [], cn = 0; function h() { eb('c1').addEventListener('keydown', function(e) { var b = eb('c1'), c = e.keyCode; if (38 == c || 40 == c) { b.autocomplete = 'off'; } 38 == c ? (++cn > hc.length && (cn = hc.length), b.value = hc[cn - 1] || '') : 40 == c ? (0 > --cn && (cn = 0), b.value = hc[cn - 1] || '') : 13 == c && (hc.length > 19 && hc.pop(), hc.unshift(b.value), cn = 0) }); } wl(h);";

const char wifi_script[] = "function c(l) {eb('s1').value = l.innerText || l.textContent; eb('p1').focus(); }";

const char script[] = "function jd() { var t = 0, i = document.querySelectorAll('input,button,textarea,select'); while (i.length >= t) { if (i[t]) { i[t]['name'] = (i[t].hasAttribute('id') && (!i[t].hasAttribute('name'))) ? i[t]['id'] : i[t]['name']; } t++; } } wl(jd); </script>";

const char style[] = "<style> div, fieldset, input, select { padding: 5px; font-size: 1em; } fieldset { background: #2F2F2F; color: #DDDDDD;} legend {float: left; font-size: 1.3em;} legend span {position: absolute; top: 10px; left: 10px;} fieldset.set1 { background: #DDDDDD; color: #2E2F2F; position: relative; padding-top: 40px;} p { margin: 0.5em 0; } input { width: 100%; box-sizing: border-box; -webkit-box-sizing: border-box; -moz-box-sizing: border-box; background: #EEEEEE; color: #2F2F2F; } input[type=checkbox], input[type=radio] { width: 1em; margin-right: 6px; vertical-align: -1px; } input[type=range] { width: 99%; } select { width: 100%; background: #EEEEEE; color: #2F2F2F; } textarea { resize: vertical; width: 98%; height: 318px; padding: 5px; overflow: auto; background: #2F2F2F; color: #FFA900; } body { text-align: center; font-family: verdana, sans-serif; background: #EEEEEE; color:#2E2F2F;} td { padding: 0px; } button { border: 0; border-radius: 0.3rem; background: #4B98D1; color: #faffff; line-height: 2.4rem; font-size: 1.2rem; width: 100%; -webkit-transition-duration: 0.4s; transition-duration: 0.4s; cursor: pointer; } button:hover { background: #025880;font-weight: bold; } .bred { background: #FFA900; } .bred:hover { background: #FF8000; font-weight: bold;} .bgrn { background: #7BB461; } .bgrn:hover { background: #3A772A;font-weight: bold; } a { color: #4B98D1; text-decoration: none; } .p { float: left; text-align: left; } .q { float: right; text-align: right; } .r { border-radius: 0.3em; padding: 2px; margin: 6px 2px; } </style></head>";

const char root_body[] = body_header "<fieldset><div style='padding:0; height:7.5em; margin-left: 15%%; white-space: pre;' id='l1' name='l1'></div></fieldset><div id=but3d style='display: block;'></div><p><form id=but3 style='display: block;' action='cn' method='get'><button>Configuration</button></form></p><p><form id=but4 style='display: block;' action='in' method='get'><button>Information</button></form></p>" button_upgrade "<p><form id=but14 style='display: block;' action='cs' method='get'><button>Console</button></form></p><p><form id=but0 style='display: block;' action='.' method='get' onsubmit='return confirm(\"Confirm Restart\");'><button name='rst' class='button bred'>Restart</button></form></p>";

const char config_body[] = body_header "" configure_1 "" configure_2 "" configure_3 "" configure_4 "" configure_5 "" configure_6 "" configure_7 "" configure_8 "<div id=but1d style='display: block;'></div><p><form id=but1 style='display: block;' action='rt' method='get' onsubmit='return confirm(\"Confirm Reset Configuration\");'><button name='non' class='button bred'>Reset Configuration</button></form>" body_footer_main_menu;

const char reset_body[] = body_header "<div style='text-align:center;'>%s</div><br><div style='text-align:center;'>Device will restart in a few seconds</div><br>" body_footer_main_menu;

const char config_cloud_body[] = body_header "<fieldset class=\"set1\"><legend><span><b>&nbsp;Cloud Configuration&nbsp;</b></span></legend><form method='get' action='cl'><p><label><input id='cl-en' type='checkbox' %s><b>Enable Cloud Connection</b></label></p><br><p><label><input id='cl-lk' type='checkbox' disabled><b>Cloud Account%s Linked</b></label></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset><p><form action='%s' method='get'><input type='hidden' name='macAddress' value='%s'/><input type='hidden' name='redirect_uri' value='%s'/><input type='hidden' name='gateway_name' value='%s'/><input type='hidden' name='uptime' value='%d'/><input type='hidden' name='RT' value='%d'/><button>Link Cloud Account</button></form></p>" body_footer_config_menu;

const char token_body[] = body_header "<div style='text-align:center;'>Link Cloud Account</div><br><div style='text-align:center;'>Cloud was successfully linked</div><br><div id=but2d style=\"display: block;\"></div><p><form id=but2 style=\"display: block;\" action='cn' method='get'><button>Configuration</button></form></p>";

const char console_body[] = body_header "<br><textarea readonly id='t1' cols='340' wrap='off'></textarea><br><br><form method='get' onsubmit='return l(1);'><input id='c1' placeholder='Enter topic and command' autofocus><br></form>" body_footer_main_menu;

const char information_body[] = body_header "<style>td {padding: 0px 5px;}</style><div id='i' name='i'></div>" body_footer_main_menu;

const char upgrade_body[] = body_header "<div id='f1' style='display:block;'><fieldset class=\"set1\"><legend><span><b>Upgrade by Web Server</b></span></legend><form method='get' action='up'><br><b>OTA URL</b><br><input id='o' placeholder=\"OTA_URL\" value=\"%s\"><br><br><button type='submit' class='button bgrn'>Start upgrade</button></form></fieldset><br><br><fieldset class=\"set1\"><legend><span><b>Upgrade to Level</b></span></legend><form method='get' action='up'><p><b>Level</b><br><select id='le'><option value='1'>Latest Release</option><option value='2'>Development</option></select></p><br><button type='submit' class='button bgrn'>Start upgrade</button></form></fieldset></div><div id='f2' style='display:none;text-align:center;'><b>Upload started ...</b></div><div id=but2d style=\"display: block;\"></div><p>" body_footer_main_menu;

const char config_wifi_body[] = body_header "%s<br><div><a href='/wi?scan='><b>Scan for all WiFi Networks</b></a></div><br><fieldset class=\"set1\"><legend><span><b>WiFi Parameters</b></span></legend><form method='get' action='wi'><p><b>WiFi Network</b> () <br><input id='s1' placeholder=\"Type or Select your WiFi Network\" value=\"%s\"></p><p><label><b>WiFi Password</b><input type='checkbox' onclick='sp(\"p1\")'></label><br><input id='p1' type='password' placeholder=\"Enter your WiFi Password\" value=\"%s\"></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset>" body_footer_config_menu;

// mqtt server (mh), mqtt port (ml), mqtt username (mu), mqtt password (mp), secure connection (sc), server certificate (msc), topic (mt)

const char config_mqtt_body[] = body_header "<fieldset class=\"set1\"><legend><span><b>MQTT Parameters</b></span></legend><form method='get' action='mq'><p><b>MQTT Server</b><br><input id='mh' placeholder=" MQTT_SERVER " value='%s'></p><p><b>MQTT Port</b><br><input id='ml' placeholder=" MQTT_PORT " value='%s'></p><p><b>MQTT Username</b><br><input id='mu' placeholder=" MQTT_USER " value='%s'></p><p><label><b>MQTT Password</b><input type='checkbox' onclick='sp(\"mp\")'></label><br><input id='mp' type='password' placeholder=\"Password\" value='%s'></p><p><b>MQTT Secure Connection</b><br><input id='sc' type='checkbox' %s></p><p><b>Gateway Name</b><br><input id='h' placeholder=" Gateway_Name " value=\"%s\"></p><p><b>MQTT Base Topic</b><br><input id='mt' placeholder='' value='%s'></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset>" body_footer_config_menu;

const char config_logging_body[] = body_header "<fieldset class=\"set1\"><legend><span><b>OpenMQTTGateway Logging</b></span></legend><form method='get' action='lo'><p><b>Log Level</b><br><select id='lo'><option %s value='0'>Silent</option><option %s value='1'>Fatal</option><option %s value='2'>Error</option><option %s value='3'>Warning</option><option %s value='4'>Notice</option><option %s value='5'>Trace</option><option %s value='6'>Verbose</option></select></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset>" body_footer_config_menu;

const char config_webui_body[] = body_header "<fieldset class=\"set1\"><legend><span><b>Configure WebUI</b></span></legend><form method='get' action='wu'><p><b>Display Metric</b><br><input id='dm' type='checkbox' %s></p><p><b>Secure WebUI</b><br><input id='sw' type='checkbox' %s></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset>" body_footer_config_menu;

const char footer[] = "<div style='text-align:right;font-size:11px;'><hr/><a href='https://community.openmqttgateway.com' target='_blank' style='color:#aaa;'>%s</a></div></div></body></html>";

#endif