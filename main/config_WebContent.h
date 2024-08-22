/* 
 OpenMQTTGateway - ESP8266 or Arduino program for home automation 

 Act as a gateway between your 433mhz, infrared IR, BLE, LoRa signal and one interface like an MQTT broker  
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
/*#if defined(ZgatewayCloud)
#  define configure_3 "<p><form action='cl' method='get'><button>Configure Cloud</button></form></p>"
#else
#  define configure_3
#endif*/
#ifndef ESPWifiManualSetup
#  define configure_3 "<p><form action='cg' method='get'><button>Configure Gateway</button></form></p>"
#else
#  define configure_3
#endif
#define configure_4 "<p><form action='wu' method='get'><button>Configure WebUI</button></form></p>"
#define configure_5 "<p><form action='lo' method='get'><button>Configure Logging</button></form></p>"
#ifdef ZgatewayLORA
#  define configure_6 "<p><form action='la' method='get'><button>Configure LORA</button></form></p>"
#elif defined(ZgatewayRTL_433) || defined(ZgatewayPilight) || defined(ZgatewayRF) || defined(ZgatewayRF2) || defined(ZactuatorSomfy)
#  define configure_6 "<p><form action='rf' method='get'><button>Configure RF</button></form></p>"
#else
#  define configure_6
#endif
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

//const char config_cloud_body[] = body_header "<fieldset class=\"set1\"><legend><span><b>&nbsp;Cloud Configuration&nbsp;</b></span></legend><form method='get' action='cl'><p><label><input id='cl-en' type='checkbox' %s><b>Enable Cloud Connection</b></label></p><br><p><label><input id='cl-lk' type='checkbox' disabled><b>Cloud Account%s Linked</b></label></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset><p><form action='%s' method='get'><input type='hidden' name='macAddress' value='%s'/><input type='hidden' name='redirect_uri' value='%s'/><input type='hidden' name='gateway_name' value='%s'/><input type='hidden' name='uptime' value='%d'/><input type='hidden' name='RT' value='%d'/><button>Link Cloud Account</button></form></p>" body_footer_config_menu;

const char token_body[] = body_header "<div style='text-align:center;'>Link Cloud Account</div><br><div style='text-align:center;'>Cloud was successfully linked</div><br><div id=but2d style=\"display: block;\"></div><p><form id=but2 style=\"display: block;\" action='cn' method='get'><button>Configuration</button></form></p>";

const char console_body[] = body_header "<br><textarea readonly id='t1' cols='340' wrap='off'></textarea><br><br><form method='get' onsubmit='return l(1);'><input id='c1' placeholder='Enter topic and command' autofocus><br></form>" body_footer_main_menu;

const char information_body[] = body_header "<style>td {padding: 0px 5px;}</style><div id='i' name='i'></div>" body_footer_main_menu;

const char upgrade_body[] = body_header "<div id='f1' style='display:block;'><fieldset class=\"set1\"><legend><span><b>Upgrade by Web Server</b></span></legend><form method='get' action='up'><br><b>OTA URL</b><br><input id='o' placeholder=\"OTA_URL\" value=\"%s\"><br><br><button type='submit' class='button bgrn'>Start upgrade</button></form></fieldset><br><br><fieldset class=\"set1\"><legend><span><b>Upgrade to Level</b></span></legend><form method='get' action='up'><p><b>Level</b><br><select id='le'><option value='1'>Latest Release</option><option value='2'>Development</option></select></p><br><button type='submit' class='button bgrn'>Start upgrade</button></form></fieldset></div><div id='f2' style='display:none;text-align:center;'><b>Upload started ...</b></div><div id=but2d style=\"display: block;\"></div><p>" body_footer_main_menu;

const char config_wifi_body[] = body_header "%s<br><div><a href='/wi?scan='><b>Scan for all WiFi Networks</b></a></div><br><fieldset class=\"set1\"><legend><span><b>WiFi Parameters</b></span></legend><form method='get' action='wi'><p><b>WiFi Network</b> () <br><input id='s1' placeholder=\"Type or Select your WiFi Network\" value=\"%s\"></p><p><label><b>WiFi Password</b></label><br><input id='p1' type='password' placeholder=\"Enter your WiFi Password\" ></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset>" body_footer_config_menu;
#ifdef ZmqttDiscovery
// mqtt server (mh), mqtt port (ml), mqtt username (mu), mqtt password (mp), secure connection (sc), server certificate (msc), mqtt topic (mt), discovery prefix (dp)
const char config_mqtt_body[] = body_header "<fieldset class=\"set1\"><legend><span><b>MQTT Parameters</b></span></legend><form method='get' action='mq'><p><b>MQTT Server</b><br><input id='mh' placeholder=" MQTT_SERVER " value='%s'></p><p><b>MQTT Port</b><br><input id='ml' placeholder=" MQTT_PORT " value='%s'></p><p><b>MQTT Username</b><br><input id='mu' placeholder=" MQTT_USER " value='%s'></p><p><label><b>MQTT Password</b></label><br><input id='mp' type='password' placeholder=\"Password\" ></p><p><b>MQTT Secure Connection</b><br><input id='sc' type='checkbox' %s></p><p><b>Gateway Name</b><br><input id='h' placeholder=" Gateway_Name " value=\"%s\"></p><p><b>MQTT Base Topic</b><br><input id='mt' placeholder='' value='%s'></p><p><b>MQTT Discovery Prefix</b><br><input id='dp' placeholder='' value='%s'></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset>" body_footer_config_menu;
#else
// mqtt server (mh), mqtt port (ml), mqtt username (mu), mqtt password (mp), secure connection (sc), server certificate (msc), mqtt topic (mt)
const char config_mqtt_body[] = body_header "<fieldset class=\"set1\"><legend><span><b>MQTT Parameters</b></span></legend><form method='get' action='mq'><p><b>MQTT Server</b><br><input id='mh' placeholder=" MQTT_SERVER " value='%s'></p><p><b>MQTT Port</b><br><input id='ml' placeholder=" MQTT_PORT " value='%s'></p><p><b>MQTT Username</b><br><input id='mu' placeholder=" MQTT_USER " value='%s'></p><p><label><b>MQTT Password</b></label><br><input id='mp' type='password' placeholder=\"Password\" ></p><p><b>MQTT Secure Connection</b><br><input id='sc' type='checkbox' %s></p><p><b>Gateway Name</b><br><input id='h' placeholder=" Gateway_Name " value=\"%s\"></p><p><b>MQTT Base Topic</b><br><input id='mt' placeholder='' value='%s'></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset>" body_footer_config_menu;
#endif
#ifndef ESPWifiManualSetup
const char config_gateway_body[] = body_header "<fieldset class=\"set1\"><legend><span><b>Gateway Configuration</b></span></legend><form method='get' action='cg'><p><b>Gateway Password (8 characters min)</b><br><input id='gp' type='password' placeholder=\"********\"  minlength='8'></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset>" body_footer_config_menu;
#endif
const char config_logging_body[] = body_header "<fieldset class=\"set1\"><legend><span><b>OpenMQTTGateway Logging</b></span></legend><form method='get' action='lo'><p><b>Log Level</b><br><select id='lo'><option %s value='0'>Silent</option><option %s value='1'>Fatal</option><option %s value='2'>Error</option><option %s value='3'>Warning</option><option %s value='4'>Notice</option><option %s value='5'>Trace</option><option %s value='6'>Verbose</option></select></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset>" body_footer_config_menu;

const char config_webui_body[] = body_header "<fieldset class=\"set1\"><legend><span><b>Configure WebUI</b></span></legend><form method='get' action='wu'><p><b>Display Metric</b><br><input id='dm' type='checkbox' %s></p><p><b>Secure WebUI</b><br><input id='sw' type='checkbox' %s></p><br><button name='save' type='submit' class='button bgrn'>Save</button></form></fieldset>" body_footer_config_menu;

const char config_rf_body[] = body_header
    "<fieldset class=\"set1\">"
    "<legend><span><b>Configure RF</b></span></legend>"
    "<form method='get' action='rf'>"

    "<p><b>Frequency</b><br>"
    "<input type='number' id='rf' name='rf' step='any' value='%.3f'></p>"

    // Active library dropdown
    "<p><b>Active library</b><br>"
    "<select id='ar' name='ar'>%s</select></p>"

    /* // Need testing
    "<p><b>OOK Threshold</b><br>"
    "<input type='number' id='oo' name='oo' step='any' value='%d'></p>"

    "<p><b>RSSI Threshold</b><br>"
    "<input type='number' id='rs' name='rs' step='any' value='%d'></p>"
*/
    "<br><button name='save' type='submit' class='button bgrn'>Save</button>"
    "</form>"
    "</fieldset>" body_footer_config_menu;

const char config_lora_body[] = body_header
    "<fieldset class=\"set1\">"
    "<legend><span><b>Configure LORA</b></span></legend>"
    "<form method='get' action='la'>"

    "<p><b>Frequency</b><br>"
    "<select id='lf' name='lf'>"
    "<option %s value='868000000'>868MHz</option>"
    "<option %s value='915000000'>915MHz</option>"
    "<option %s value='433000000'>433MHz</option>"
    "</select></p>"

    "<p><b>TX Power</b><br>"
    "<select id='lt' name='lt'>"
    "<option %s value='0'>0 dBm</option>"
    "<option %s value='1'>1 dBm</option>"
    "<option %s value='2'>2 dBm</option>"
    "<option %s value='3'>3 dBm</option>"
    "<option %s value='4'>4 dBm</option>"
    "<option %s value='5'>5 dBm</option>"
    "<option %s value='6'>6 dBm</option>"
    "<option %s value='7'>7 dBm</option>"
    "<option %s value='8'>8 dBm</option>"
    "<option %s value='9'>9 dBm</option>"
    "<option %s value='10'>10 dBm</option>"
    "<option %s value='11'>11 dBm</option>"
    "<option %s value='12'>12 dBm</option>"
    "<option %s value='13'>13 dBm</option>"
    "<option %s value='14'>14 dBm</option>"
    "</select></p>"

    "<p><b>Spreading Factor</b><br>"
    "<select id='ls' name='ls'>"
    "<option %s value='7'>SF7</option>"
    "<option %s value='8'>SF8</option>"
    "<option %s value='9'>SF9</option>"
    "<option %s value='10'>SF10</option>"
    "<option %s value='11'>SF11</option>"
    "<option %s value='12'>SF12</option>"
    "</select></p>"

    "<p><b>Signal Bandwidth</b><br>"
    "<select id='lb' name='lb'>"
    "<option %s value='7800'>7.8 kHz</option>"
    "<option %s value='10400'>10.4 kHz</option>"
    "<option %s value='15600'>15.6 kHz</option>"
    "<option %s value='20800'>20.8 kHz</option>"
    "<option %s value='31250'>31.25 kHz</option>"
    "<option %s value='41700'>41.7 kHz</option>"
    "<option %s value='62500'>62.5 kHz</option>"
    "<option %s value='125000'>125 kHz</option>"
    "<option %s value='250000'>250 kHz</option>"
    "<option %s value='500000'>500 kHz</option>"
    "</select></p>"

    "<p><b>Coding Rate</b><br>"
    "<select id='lc' name='lc'>"
    "<option %s value='5'>4/5</option>"
    "<option %s value='6'>4/6</option>"
    "<option %s value='7'>4/7</option>"
    "<option %s value='8'>4/8</option>"
    "</select></p>"

    "<p><b>Preamble Length</b><br>"
    "<input type='number' id='ll' name='ll' value='%d'></p>"

    "<p><b>Sync Word</b><br>"
    "<input type='text' id='lw' name='lw' value='0x%02X'></p>"

    "<p><b>CRC</b><br>"
    "<input type='checkbox' id='lr' name='lr' %s></p>"

    "<p><b>Invert IQ</b><br>"
    "<input type='checkbox' id='li' name='li' %s></p>"

    "<p><b>Only known</b><br>"
    "<input type='checkbox' id='ok' name='ok' %s></p>"

    "<br><button name='save' type='submit' class='button bgrn'>Save</button>"
    "</form>"
    "</fieldset>" body_footer_config_menu;

const char footer[] = "<div style='text-align:right;font-size:11px;'><hr/><a href='https://community.openmqttgateway.com' target='_blank' style='color:#aaa;'>%s</a></div></div></body></html>";

// Source file - https://github.com/1technophile/OpenMQTTGateway/blob/54decb4b65c7894b926ac3a89de0c6b2a3021506/docs/.vuepress/public/favicon-16x16.png
// Workflow was, convert to ICO format using an online convertor, then use the desktop utility xxd to convert to byte array

const unsigned char Openmqttgateway_logo_mini_ico[] = {
    0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x10, 0x00, 0x00, 0x01, 0x00,
    0x20, 0x00, 0x68, 0x04, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0xa8, 0x6a, 0x00, 0x4f, 0xa8,
    0x6a, 0x0a, 0x4f, 0xa8, 0x6a, 0x62, 0x4f, 0xa8, 0x6a, 0xb0, 0x4f, 0xa8,
    0x6a, 0xbc, 0x4f, 0xa8, 0x6a, 0xb0, 0x4f, 0xa8, 0x6a, 0x61, 0x4f, 0xa8,
    0x6a, 0x0a, 0x4f, 0xa8, 0x6a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0xa8, 0x6a, 0x00, 0x4f, 0xa8,
    0x6a, 0x03, 0x4f, 0xa8, 0x6a, 0x6a, 0x4f, 0xa8, 0x6a, 0x9a, 0x4f, 0xa8,
    0x6a, 0x41, 0x4f, 0xa8, 0x6a, 0x22, 0x4f, 0xa8, 0x6a, 0x41, 0x4f, 0xa8,
    0x6a, 0x9a, 0x4f, 0xa8, 0x6a, 0x68, 0x4f, 0xa8, 0x6a, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0xa8,
    0x6a, 0x00, 0x4f, 0xa8, 0x6a, 0x2f, 0x4f, 0xa8, 0x6a, 0xa3, 0x4f, 0xa8,
    0x6a, 0x24, 0x4f, 0xa8, 0x6a, 0x00, 0x4f, 0xa8, 0x6a, 0x00, 0x4f, 0xa8,
    0x6a, 0x00, 0x4f, 0xa8, 0x6a, 0x25, 0x4f, 0xa8, 0x6a, 0xa2, 0x4f, 0xa8,
    0x6a, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0xa8,
    0x6a, 0x00, 0x4f, 0xa8, 0x6a, 0x00, 0x4f, 0xa8, 0x6a, 0x5d, 0x4f, 0xa8,
    0x6a, 0x7e, 0x4f, 0xa8, 0x6a, 0x01, 0x4f, 0xa8, 0x6a, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x4f, 0xa8, 0x6a, 0x00, 0x4f, 0xa8, 0x6a, 0x02, 0x4f, 0xa8,
    0x6a, 0x7e, 0x4f, 0xa8, 0x6a, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x4f, 0xa8, 0x6a, 0x00, 0x4f, 0xa8, 0x6a, 0x00, 0x4f, 0xa8,
    0x6a, 0x64, 0x4f, 0xa8, 0x6a, 0x75, 0x4f, 0xa8, 0x6a, 0x00, 0x4f, 0xa8,
    0x6a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0xa8, 0x6a, 0x00, 0x4f, 0xa8,
    0x6a, 0x00, 0x4f, 0xa8, 0x6a, 0x74, 0x4f, 0xa8, 0x6a, 0x6f, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0xa8,
    0x6a, 0x00, 0x4e, 0xa8, 0x6b, 0x42, 0x4f, 0xa8, 0x6a, 0x98, 0x4f, 0xa8,
    0x6a, 0x0f, 0x4f, 0xa8, 0x6a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0xa8,
    0x6a, 0x00, 0x4f, 0xa8, 0x6a, 0x10, 0x4f, 0xa8, 0x6a, 0x98, 0x4f, 0xa8,
    0x6a, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x85,
    0x3d, 0x00, 0xcf, 0x82, 0x3a, 0x0c, 0xb3, 0x8b, 0x44, 0x50, 0x60, 0xa3,
    0x63, 0xa6, 0x4e, 0xa8, 0x6a, 0x72, 0x4e, 0xa8, 0x6a, 0x14, 0x4c, 0xa9,
    0x6b, 0x05, 0x4f, 0xa8, 0x6a, 0x14, 0x4f, 0xa8, 0x6a, 0x74, 0x4f, 0xa8,
    0x6a, 0x8c, 0x4f, 0xa8, 0x6a, 0x0e, 0x00, 0x99, 0xff, 0x00, 0x00, 0x99,
    0xff, 0x0b, 0x00, 0x99, 0xff, 0x25, 0x00, 0x99, 0xff, 0x1e, 0x27, 0x95,
    0xd9, 0x05, 0xc8, 0x85, 0x3b, 0x28, 0xc6, 0x85, 0x3d, 0x6e, 0xc8, 0x84,
    0x3c, 0x55, 0x70, 0x9f, 0x5e, 0x2b, 0x4e, 0xa8, 0x6a, 0x8a, 0x4f, 0xa8,
    0x6a, 0x9f, 0x4f, 0xa8, 0x6a, 0x8d, 0x4f, 0xa8, 0x6a, 0x9f, 0x4f, 0xa8,
    0x6a, 0x8a, 0x4f, 0xa8, 0x6a, 0x1f, 0x4f, 0xa8, 0x6a, 0x00, 0x00, 0x99,
    0xff, 0x19, 0x00, 0x99, 0xff, 0x63, 0x00, 0x99, 0xff, 0x60, 0x00, 0x99,
    0xff, 0x66, 0x25, 0x95, 0xda, 0x61, 0xbc, 0x86, 0x47, 0x65, 0xc8, 0x85,
    0x3b, 0x31, 0xc6, 0x85, 0x3d, 0x03, 0x73, 0x9d, 0x5c, 0x00, 0x39, 0xae,
    0x72, 0x09, 0x83, 0x98, 0x56, 0x59, 0x7c, 0x9a, 0x59, 0x76, 0x4b, 0xa9,
    0x6b, 0x31, 0x4f, 0xa8, 0x6a, 0x0a, 0x4f, 0xa8, 0x6a, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x99, 0xff, 0x5c, 0x00, 0x99, 0xff, 0x43, 0x00, 0x99,
    0xff, 0x01, 0x00, 0x99, 0xff, 0x0a, 0x06, 0x99, 0xf9, 0x66, 0x1d, 0x96,
    0xe2, 0x33, 0x00, 0x9b, 0xff, 0x00, 0xc6, 0x85, 0x3d, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xc8, 0x84, 0x3c, 0x00, 0xc8, 0x84, 0x3c, 0x4d, 0xc9, 0x84,
    0x3c, 0x44, 0xc9, 0x84, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x99, 0xff, 0x6c, 0x00, 0x99,
    0xff, 0x26, 0x00, 0x99, 0xff, 0x00, 0x00, 0x99, 0xff, 0x00, 0x00, 0x99,
    0xff, 0x4f, 0x00, 0x99, 0xff, 0x3b, 0x00, 0x99, 0xff, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xc7, 0x85, 0x39, 0x00, 0xbc, 0x85, 0x64, 0x00, 0xc6, 0x85,
    0x3d, 0x5e, 0xc6, 0x85, 0x3d, 0x34, 0xc6, 0x85, 0x3d, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x99,
    0xff, 0x45, 0x00, 0x99, 0xff, 0x5f, 0x00, 0x99, 0xff, 0x1d, 0x00, 0x99,
    0xff, 0x2d, 0x00, 0x99, 0xff, 0x6c, 0x00, 0x99, 0xff, 0x1a, 0x00, 0x99,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbc, 0x82, 0x60, 0x00, 0xb5, 0x81,
    0x78, 0x06, 0xc3, 0x84, 0x48, 0x64, 0xc4, 0x84, 0x44, 0x23, 0xc4, 0x84,
    0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x99, 0xff, 0x07, 0x00, 0x99, 0xff, 0x40, 0x00, 0x99,
    0xff, 0x64, 0x00, 0x99, 0xff, 0x60, 0x00, 0x99, 0xff, 0x29, 0x00, 0x99,
    0xff, 0x00, 0x00, 0x99, 0xff, 0x00, 0xa0, 0x7b, 0xc2, 0x00, 0xa0, 0x7b,
    0xc3, 0x0a, 0xa1, 0x7b, 0xc0, 0x42, 0xa7, 0x7d, 0xaa, 0x61, 0xa3, 0x7c,
    0xb8, 0x40, 0x9e, 0x7b, 0xc8, 0x06, 0xa0, 0x7b, 0xc2, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x99, 0xff, 0x00, 0x00, 0x99,
    0xff, 0x00, 0x00, 0x99, 0xff, 0x06, 0x00, 0x99, 0xff, 0x04, 0x00, 0x99,
    0xff, 0x00, 0x00, 0x99, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x7b,
    0xc2, 0x00, 0xa0, 0x7b, 0xc2, 0x33, 0xa0, 0x7b, 0xc2, 0x3e, 0x9d, 0x7a,
    0xce, 0x0c, 0xa0, 0x7b, 0xc3, 0x47, 0xa0, 0x7b, 0xc2, 0x27, 0xa0, 0x7b,
    0xc2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xa0, 0x7b, 0xc2, 0x00, 0xa0, 0x7b, 0xc2, 0x35, 0xa0, 0x7b,
    0xc2, 0x39, 0xa0, 0x7b, 0xc2, 0x07, 0xa0, 0x7b, 0xc2, 0x43, 0xa0, 0x7b,
    0xc2, 0x2a, 0xa0, 0x7b, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x7b, 0xc2, 0x00, 0xa0, 0x7b,
    0xc2, 0x0e, 0xa0, 0x7b, 0xc2, 0x52, 0xa0, 0x7b, 0xc2, 0x60, 0xa0, 0x7b,
    0xc2, 0x4b, 0xa0, 0x7b, 0xc2, 0x09, 0xa0, 0x7b, 0xc2, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x01, 0x00, 0x00, 0xfe, 0x00,
    0x00, 0x00, 0xfe, 0x38, 0x00, 0x00, 0xfe, 0x38, 0x00, 0x00, 0xfe, 0x7c,
    0x00, 0x00, 0xfe, 0x38, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x80, 0x01,
    0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x03, 0xcf, 0x00, 0x00, 0x33, 0xcf,
    0x00, 0x00, 0x03, 0x8f, 0x00, 0x00, 0x07, 0x07, 0x00, 0x00, 0xcf, 0x07,
    0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00};
unsigned int Openmqttgateway_logo_mini_ico_len = 1150;

#endif
