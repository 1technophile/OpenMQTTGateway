var oWebsocket = false;
var bConnected = false;
var bInitialized = false;
var bSending = false;
var sHTTPProtocol = "http";
var sWSProtocol = "ws";
var aDecimals = new Array();
var aDateTime = new Array();
var aDimLevel = new Array();
var aPollInterval = new Array();
var aMarqueueInterval = new Array();
var aReadOnly = new Array();
var aStates = new Array();
var bShowTabs = true;
var bStatsEnabled = true;
var iPLVersion = 0;
var iPLNVersion = 0;
var iFWVersion = 0;
var aTimers = new Array();
var sDateTimeFormat = "HH:mm:ss YYYY-MM-DD";
var aDateTimeFormats = new Array();
var aWebcamUrl = new Array();
var aDecimalTypes = ["temperature", "humidity", "wind", "pressure", "sunriseset", "illuminance"];
var userLang = navigator.language || navigator.userLanguage;
var language;

if(document.location.protocol == "https:") {
	sHTTPProtocol = "https";
	sWSProtocol = "wss";
}

var language_en = {
	off: "Off",
	on: "On",
	stopped: "Stopped",
	started: "Started",
	toggling: "Toggling",
	up: "Up",
	down: "Down",
	update: "Update",
	loading: "Loading",
	available: "available",
	confirm: "Are you sure?",
	connecting: "Connecting",
	connection_lost: "Connection lost, touch to reload",
	connection_failed: "Failed to connect, touch to reload",
	unexpected_error: "An unexpected error occured",
	insecure_certificate: "You are using the default pilight.pem certificate. This results in a highly insecure https connection! Please personalize your certificate to remove this message."
}

var language_de = {
	off: "Aus",
	on: "Ein",
	stopped: "Gestoppt",
	started: "Gestartet",
	toggling: "schaltend",
	up: "+",
	down: "-",
	update: "Aktualisieren",
	loading: "ladend",
	available: "verfügbar",
	confirm: "Bist du sicher?",
	connecting: "Verbindung wird aufgebaut",
	connection_lost: "Verbindung verloren! Hier berühren, um die Seite neu zu laden.",
	connection_failed: "Verbindung fehlgeschlagen! Hier berühren, um die Seite neu zu laden.",
	unexpected_error: "Es ist ein unerwarteter Fehler aufgetreten.",
	insecure_certificate: "You are using the default {0} certificate. This is a highly insecure way of using https connections! Please personalize your certificate to remove this message."
}

var language_nl = {
	off: "Uit",
	on: "Aan",
	stopped: "Gestopt",
	started: "Gestart",
	toggling: "Omzetten",
	up: "Omhoog",
	down: "Omlaag",
	update: "Bijwerken",
	loading: "Verbinding maken",
	available: "beschikbaar",
	confirm: "Weet u dat zeker?",
	connection_lost: "Verbinding verloren, klik om te herladen",
	connection_failed: "Kan niet verbinden, klik om te herhalen",
	unexpected_error: "Er heeft zich een onverwachte fout voorgedaan",
	insecure_certificate: "You are using the default {0} certificate. This is a highly insecure way of using https connections! Please personalize your certificate to remove this message."
}

var language_fr = {
	off: "Eteint",
	on: "Allumé",
	stopped: "Arrêté",
	started: "Démarré",
	toggling: "En cours",
	up: "Haut",
	down: "Bas",
	update: "Mise à jour",
	loading: "Chargement en cours",
	available: "Disponible",
	confirm: "Êtes-vous sûr",
	connecting: "Connexion en cours",
	connection_lost: "Connexion perdue, appuyez pour recharger",
	connection_failed: "Connexion impossible, appuyez pour réessayer",
	unexpected_error: "Une erreur inattendue s'est produite",
	insecure_certificate: "You are using the default {0} certificate. This is a highly insecure way of using https connections! Please personalize your certificate to remove this message."
}

if(userLang.indexOf('nl') != -1) {
	language = language_nl;
}
else if(userLang.indexOf('de') != -1){
	language = language_de;
}
else if(userLang.indexOf('fr') != -1){
	language = language_fr;
}
else {
	language = language_en;
}

String.prototype.format = function() {
    var formatted = this;
    for (var i = 0; i < arguments.length; i++) {
        var regexp = new RegExp('\\{'+i+'\\}', 'gi');
        formatted = formatted.replace(regexp, arguments[i]);
    }
    return formatted;
};

function alphaNum(string) {
	return string.replace(/\W/g, '');
}

var iLatestTap1 = 0;
var iLatestTap2 = 0;

function toggleTabs() {
	if(oWebsocket) {
		if(bShowTabs) {
			var json = '{"action":"registry","type":"set","key":"webgui.tabs","value":0}';
		} else {
			var json = '{"action":"registry","type":"set","key":"webgui.tabs","value":1}';
		}
		oWebsocket.send(json);
		document.location = document.location;
	} else {
		bSending = true;
		if(bShowTabs) {
			$.get(sHTTPProtocol+'://'+location.host+'/registry?type=set&key=webgui.tabs&value=0');
		} else {
			$.get(sHTTPProtocol+'://'+location.host+'/registry?type=set&key=webgui.tabs&value=1');
		}
		$.mobile.loading('show', {
			'text': '',
			'textVisible': true,
			'theme': 'b'
		});		
		window.setTimeout(function() {
			document.location = document.location;
		}, 1000);
	}
}

$(document).click(function(e) {
	if($('#helpdlg').length == 1) {
		$('#helpdlg').remove();
	}
});

$(document).mouseup(function(e) {
	if(e.target.className.indexOf('ui-page') == 0 || e.target.className.indexOf('ui-content') == 0) {
		iDate = new Date().getTime();
		if((iLatestTap1-iDate) > 2000) {
			iLatestTap1 = 0;
			iLatestTap2 = 0;
		}
		if(iLatestTap1 == 0) {
			iLatestTap1=new Date().getTime();
		} else if(iLatestTap2 == 0) {
			iLatestTap2=new Date().getTime();
			if((iLatestTap2-iLatestTap1) > 250 && (iLatestTap2-iLatestTap1) < 1000) {
				toggleTabs();
			}
			iLatestTap1 = 0;
			iLatestTap2 = 0;
		}
	}
});

$(document).keydown(function(e) {
	if(e.keyCode == 72) {
		if($('#helpdlg').length == 1) {
			$('#helpdlg').remove();
		} else {
			$("<div data-theme='a' id='helpdlg' class='ui-loader ui-overlay-shadow ui-body-e ui-corner-all'><b>Short Cuts</b><br /><ul><li>[ctrl] + [alt] + [shift] + t<br />Switch between tabular or vertical view</li></ul></div>")
			.appendTo($('body'));
			$('#helpdlg').css({ "display": "block", "padding": "10px", "width": "300px", "position": "relative", "left" : ($(window).width() / 2)-150+'px', "top" : (($(window).height() / 2)-($('#helpdlg').height() / 2))+'px'})
		}
	} else {
		if($('#helpdlg').length == 1) {
			$('#helpdlg').remove();
		}
	}
	if(e.keyCode == 84 && e.altKey && e.ctrlKey && e.shiftKey) {
		toggleTabs();
	}
});

function createLabelElement(sTabId, sDevId, aValues) {
	if($('#'+sDevId).length == 0) {
		if(bShowTabs) {
			oTab = $('#'+sTabId).find('ul');
		} else {
			oTab = $('#all');
		}
		if('name' in aValues) {
			oTab.append($('<li id="'+sDevId+'" class="label" data-icon="false"><div class="name">'+aValues['name']+'</div><div class="marquee"><div class="text">'+aValues['label']+'</div></div></li>'));
			$('#'+sDevId+' div.marquee .text').css('color', aValues['color']);
			// var mar = $('#'+sDevId+' div.marquee .text');
			// var left = -(mar.width()+3);
			// var ori = left;
			// var cWidth = $('#'+sDevId+' div.marquee').width()+50;
			// if(-cWidth > left) {
				// mar.marquee = function() {
					// mar.css('right', left);
					// left++;
					// if(left > cWidth) {
						// left = ori;
					// }
				// };
				// aMarqueueInterval[sDevId]=setInterval(mar.marquee, 10);
			// }
		}
		oTab.listview();
		oTab.listview("refresh");
	}
}

function createSwitchElement(sTabId, sDevId, aValues) {
	if($('#'+sDevId+'_switch').length == 0) {
		if(bShowTabs) {
			oTab = $('#'+sTabId).find('ul');
		} else {
			oTab = $('#all');
		}
		if('name' in aValues) {
			oTab.append($('<li id="'+sDevId+'" class="switch" data-icon="false"><div class="name">'+aValues['name']+'</div><select id="'+sDevId+'_switch" data-role="slider"><option value="off">'+language.off+'</option><option value="on">'+language.on+'</option></select></li>'));
		}
		$('#'+sDevId+'_switch').slider();
		$('#'+sDevId+'_switch').bind("change", function(event, ui) {
			event.stopPropagation();
			if('confirm' in aValues && aValues['confirm']) {
				if(window.confirm("Are you sure?") == false) {
					if(this.value == "off") {
						$('#'+sDevId+'_switch')[0].selectedIndex = 1;
					} else {
						$('#'+sDevId+'_switch')[0].selectedIndex = 0;
					}
					$('#'+sDevId+'_switch').slider('refresh');
					return false;
				}
			}

			if(oWebsocket) {
				if('all' in aValues && aValues['all'] == 1) {
					var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+this.value+'","values":{"all": 1}}}';
				} else {
					var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+this.value+'"}}';
				}
				oWebsocket.send(json);
			} else {
				bSending = true;
				if('all' in aValues && aValues['all'] == 1) {
					$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&state='+this.value+'&values[all]=1');
				} else {
					$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&state='+this.value);
				}
				window.setTimeout(function() { bSending = false; }, 1000);
			}
		});
		oTab.listview();
		oTab.listview("refresh");
	}
	if('readonly' in aValues && aValues['readonly']) {
		aReadOnly[sDevId] = 1;
		$('#'+sDevId+'_switch').slider('disable');
	} else {
		aReadOnly[sDevId] = 0;
	}
}

function createPendingSwitchElement(sTabId, sDevId, aValues) {
	if($('#'+sDevId+'_pendingsw').length == 0) {
		if(bShowTabs) {
			oTab = $('#'+sTabId).find('ul');
		} else {
			oTab = $('#all');
		}

		if('name' in aValues) {
			oTab.append($('<li id="'+sDevId+'" class="pendingsw" data-icon="false"><div class="name">'+aValues['name']+'</div><a data-role="button" data-inline="true" data-mini="true" data-icon="on" id="'+sDevId+'_pendingsw">&nbsp;</a></li>'));
		}
		$('#'+sDevId+'_pendingsw').button();
		$('#'+sDevId+'_pendingsw').bind("click", function(event, ui) {
			event.stopPropagation();
			if('confirm' in aValues && aValues['confirm']) {
				if(window.confirm("Are you sure?") == false) {
					return false;
				}
			}
			$('#'+sDevId+'_pendingsw').parent().removeClass('ui-icon-on').removeClass('ui-icon-off').addClass('ui-icon-loader');
			$('#'+sDevId+'_pendingsw').button('disable');
			$('#'+sDevId+'_pendingsw').text(language.toggling);
			$('#'+sDevId+'_pendingsw').button('refresh');
			
			if(oWebsocket) {
				var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+((aStates[sDevId] == "off") ? "on" : "off")+'"}}';
				oWebsocket.send(json);
			} else {
				bSending = true;
				$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&state='+((aStates[sDevId] == "off") ? "on" : "off"));
				window.setTimeout(function() { bSending = false; }, 1000);
			}
		});
		$('#'+sDevId).bind("click", function(event, ui) {
			if(!$('#'+sDevId+'_pendingsw').prop("disabled")) {
				event.stopPropagation();
				if('confirm' in aValues && aValues['confirm']) {
					if(window.confirm("Are you sure?") == false) {
						return false;
					}
				}
				$('#'+sDevId+'_pendingsw').parent().removeClass('ui-icon-on').removeClass('ui-icon-off').addClass('ui-icon-loader');
				$('#'+sDevId+'_pendingsw').button('disable');
				$('#'+sDevId+'_pendingsw').text(language.toggling);
				$('#'+sDevId+'_pendingsw').button('refresh');
				if(oWebsocket) {
					var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+((aStates[sDevId] == "off") ? "on" : "off")+'"}}';
					oWebsocket.send(json);
				} else {
					bSending = true;
					$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&state='+((aStates[sDevId] == "off") ? "on" : "off"));
					window.setTimeout(function() { bSending = false; }, 1000);
				}
			}
		});
		oTab.listview();
		oTab.listview("refresh");
	}
	if('readonly' in aValues && aValues['readonly']) {
		aReadOnly[sDevId] = 1;
		$('#'+sDevId+'_pendingsw').button('disable');
	} else {
		aReadOnly[sDevId] = 0;
	}
}

function createScreenElement(sTabId, sDevId, aValues) {
	if($('#'+sDevId+'_screen').length == 0) {
		if(bShowTabs) {
			oTab = $('#'+sTabId).find('ul');
		} else {
			oTab = $('#all');
		}
		if('name' in aValues) {
			oTab.append($('<li  id="'+sDevId+'" class="screen" data-icon="false"><div class="name">'+aValues['name']+'</div><div id="'+sDevId+'_screen" class="screen" data-role="fieldcontain" data-type="horizontal"><fieldset data-role="controlgroup" class="controlgroup" data-type="horizontal" data-mini="true"><input type="radio" name="'+sDevId+'_screen" id="'+sDevId+'_screen_down" value="down" /><label for="'+sDevId+'_screen_down">'+language.down+'</label><input type="radio" name="'+sDevId+'_screen" id="'+sDevId+'_screen_up" value="up" /><label for="'+sDevId+'_screen_up">'+language.up+'</label></fieldset></div></li>'));
		}
		$("div").trigger("create");
		$('#'+sDevId+'_screen_down').checkboxradio();
		$('#'+sDevId+'_screen_up').checkboxradio();
		$('#'+sDevId+'_screen_down').bind("change", function(event, ui) {
			event.stopPropagation();
			if('confirm' in aValues && aValues['confirm']) {
				if(window.confirm("Are you sure?") == false) {
					return false;
				}
			}
			i = 0;
			oLabel = this.parentNode.getElementsByTagName('label')[0];
			$(oLabel).removeClass('ui-btn-active');
			x = window.setInterval(function() {
				i++;
				if(i%2 == 1)
					$(oLabel).removeClass('ui-btn-active');
				else
					$(oLabel).addClass('ui-btn-active');
				if(i==2)
					window.clearInterval(x);
			}, 100);

			if(oWebsocket) {
				if('all' in aValues && aValues['all'] == 1) {
					var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+this.value+'","values":{"all": 1}}}';
				} else {
					var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+this.value+'"}}'
				}
				oWebsocket.send(json);
			} else {
				bSending = true;
				if('all' in aValues && aValues['all'] == 1) {
					$.get(sHTTPProtocol+'://'+location.host+'/control/control?device='+sDevId+'&state='+this.value+'&values[all]=1');
				} else {
					$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&state='+this.value);
				}
				window.setTimeout(function() { bSending = false; }, 1000);
			}
		});
		$('#'+sDevId+'_screen_up').bind("change", function(event, ui) {
			event.stopPropagation();
			if('confirm' in aValues && aValues['confirm']) {
				if(window.confirm("Are you sure?") == false) {
					return false;
				}
			}
			i = 0;
			oLabel = this.parentNode.getElementsByTagName('label')[0];
			$(oLabel).removeClass('ui-btn-active');
			x = window.setInterval(function() {
				i++;
				if(i%2 == 1)
					$(oLabel).removeClass('ui-btn-active');
				else
					$(oLabel).addClass('ui-btn-active');
				if(i==2)
					window.clearInterval(x);
			}, 100);

			if(oWebsocket) {
				if('all' in aValues && aValues['all'] == 1) {
					var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+this.value+'","values":{"all": 1}}}';
				} else {
					var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+this.value+'"}}'
				}
				oWebsocket.send(json);
			} else {
				bSending = true;
				if('all' in aValues && aValues['all'] == 1) {
					$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&state='+this.value+'&values[all]=1');
				} else {
					$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&state='+this.value);
				}
				window.setTimeout(function() { bSending = false; }, 1000);
			}
		});
	}
	oTab.listview();
	oTab.listview("refresh");
	if('readonly' in aValues && aValues['readonly']) {
		aReadOnly[sDevId] = 1;
		$('#'+sDevId+'_screen_up').checkboxradio('disable');
		$('#'+sDevId+'_screen_down').checkboxradio('disable');
	} else {
		aReadOnly[sDevId] = 0;
	}
}

function createDimmerElement(sTabId, sDevId, aValues) {
	aDimLevel[sDevId] = 0;
	if($('#'+sDevId+'_switch').length == 0) {
		if(bShowTabs) {
			oTab = $('#'+sTabId).find('ul');
		} else {
			oTab = $('#all');
		}
		if('name' in aValues && 'dimlevel-minimum' in aValues && 'dimlevel-maximum' in aValues) {
			oTab.append($('<li id="'+sDevId+'" class="dimmer" data-icon="false"><div class="name">'+aValues['name']+'</div><select id="'+sDevId+'_switch" data-role="slider"><option value="off">'+language.off+'</option><option value="on">'+language.on+'</option></select><div id="'+sDevId+'_dimmer" min="'+aValues['dimlevel-minimum']+'" max="'+aValues['dimlevel-maximum']+'" data-highlight="true" ><input type="value" id="'+sDevId+'_value" class="slider-value dimmer-slider ui-slider-input ui-input-text ui-body-c ui-corner-all ui-shadow-inset" disabled="true"/></div></li>'));
		}
		$('#'+sDevId+'_switch').slider();
		$('#'+sDevId+'_switch').bind("change", function(event, ui) {
			event.stopPropagation();
			if('confirm' in aValues && aValues['confirm']) {
				if(window.confirm("Are you sure?") == false) {
					if(this.value == "off") {
						$('#'+sDevId+'_switch')[0].selectedIndex = 1;
					} else {
						$('#'+sDevId+'_switch')[0].selectedIndex = 0;
					}
					$('#'+sDevId+'_switch').slider('refresh');
					return false;
				}
			}

			if(oWebsocket) {
				if('all' in aValues && aValues['all'] == 1) {
					var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+this.value+'","values":{"all":1,"dimlevel":'+$('#'+sDevId+'_dimmer').val()+'}}}';
				} else {
					if(this.value == "on") {
						var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+this.value+'","values":{"dimlevel":'+$('#'+sDevId+'_dimmer').val()+'}}}';
					} else {
						var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"'+this.value+'"}}';
					}
				}
				oWebsocket.send(json);
			} else {
				bSending = true;
				if('all' in aValues && aValues['all'] == 1) {
					$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&state='+this.value+'&values[all]=1&values[dimlevel]='+$('#'+sDevId+'_dimmer').val());
				} else {
					$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&state='+this.value+'&values[dimlevel]='+$('#'+sDevId+'_dimmer').val());
				}
				window.setTimeout(function() { bSending = false; }, 1000);
			}
		});

		$('#'+sDevId+'_dimmer').slider({
			stop: function() {
				if('confirm' in aValues && aValues['confirm']) {
					if(window.confirm("Are you sure?") == false) {
						$('#'+sDevId+'_value').val(aDimLevel[sDevId]);
						$('#'+sDevId+'_dimmer').val(aDimLevel[sDevId]);
						$('#'+sDevId+'_dimmer').slider('refresh');
						return false;
					}
				}
				if(aDimLevel[sDevId] != this.value) {
					aDimLevel[sDevId] = this.value;
					$('#'+sDevId+'_switch')[0].selectedIndex = 1;
					$('#'+sDevId+'_switch').slider('refresh');
					if(oWebsocket) {
						var json = '{"action":"control","code":{"device":"'+sDevId+'","state":"on","values":{"dimlevel":'+this.value+'}}}';
						oWebsocket.send(json);
					} else {
						bSending = true;
						$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&state=on&values[dimlevel]='+this.value);
						window.setTimeout(function() { bSending = false; }, 1000);
					}
				}
			}
		});
		$('#'+sDevId+'_dimmer').change(function(event) {
			$('#'+sDevId+'_value').val(this.value);
		});

		oTab.listview();
		oTab.listview("refresh");
	}
	$('#'+sDevId+'_dimmer').slider('refresh');
	if('readonly' in aValues && aValues['readonly']) {
		aReadOnly[sDevId] = 1;
		$('#'+sDevId+'_switch').slider('disable');
		$('#'+sDevId+'_dimmer').slider('disable');
	} else {
		aReadOnly[sDevId] = 0;
	}
}

function createWeatherElement(sTabId, sDevId, aValues) {
	aPollInterval[sDevId] = new Array();
	$.each(aDecimalTypes, function(index, value) {
		if(!(sDevId in aDecimals)) {
			aDecimals[sDevId] = new Array();
		}
		if(value+'-decimals' in aValues) {
			aDecimals[sDevId][value] = aValues[value+'-decimals'];
		} else {
			aDecimals[sDevId][value] = 0;
		}
	});
	if('poll-interval' in aValues) {
		aPollInterval[sDevId] = aValues['poll-interval'];
	}

	if($('#'+sDevId+'_weather').length == 0) {
		if(bShowTabs) {
			oTab = $('#'+sTabId).find('ul');
		} else {
			oTab = $('#all');
		}
		if('name' in aValues) {
			oTab.append($('<li class="weather" id="'+sDevId+'_weather" data-icon="false"><div class="name">'+aValues['name']+'</div><div class="weather_values" id="'+sDevId+'_weather_values" /></li>'));
		}
		if('show-update' in aValues && aValues['show-update']) {
			oTab.find('#'+sDevId+'_weather_values').append($('<div class="update_inactive" id="'+sDevId+'_upd" title="'+language.update+'">&nbsp;</div>'));
			$('#'+sDevId+'_upd').click(function() {
				if(this.className.indexOf('update_active') == 0) {
					if(oWebsocket) {
						var json = '{"action":"control","code":{"device":"'+sDevId+'","values":{"update":1}}}';
						oWebsocket.send(json);
					} else {
						bSending = true;
						$.get(sHTTPProtocol+'://'+location.host+'/control?device='+sDevId+'&values[update]=1');
						window.setTimeout(function() { bSending = false; }, 1000);
					}
				}
			});
		}
		if('show-battery' in aValues && aValues['show-battery'] && 'battery' in aValues) {
			oTab.find('#'+sDevId+'_weather_values').append($('<div id="'+sDevId+'_batt" class="battery green"></div>'));
		}
		if('show-rain' in aValues && aValues['show-rain']) {
			oTab.find('#'+sDevId+'_weather_values').append($('<div class="rain_icon"></div><div class="rain" id="'+sDevId+'_rain"></div>'));
		}
		if('show-wind' in aValues && aValues['show-wind']) {
			oTab.find('#'+sDevId+'_weather_values').append($('<div class="windavg_icon"></div><div class="windavg" id="'+sDevId+'_windavg"></div>'));
			oTab.find('#'+sDevId+'_weather_values').append($('<div class="windgust_icon"></div><div class="winddir_icon" id="'+sDevId+'_winddir"></div><div class="windgust" id="'+sDevId+'_windgust"></div>'));
			$('#'+sDevId+'_weather .winddir_icon').css({transform: 'rotate(' + aValues['winddir'] + 'deg)'});
		}
		if('show-humidity' in aValues && aValues['show-humidity']) {
			oTab.find('#'+sDevId+'_weather_values').append($('<div class="humidity_icon"></div><div class="humidity" id="'+sDevId+'_humi"></div>'));
		}
		if('show-temperature' in aValues && aValues['show-temperature']) {
			oTab.find('#'+sDevId+'_weather_values').append($('<div class="temperature_icon"></div><div class="temperature" id="'+sDevId+'_temp"></div>'));
		}
		if('show-pressure' in aValues && aValues['show-pressure']) {
			oTab.find('#'+sDevId+'_weather_values').append($('<div class="pressure_icon"></div><div class="pressure" id="'+sDevId+'_pres"></div>'));
		}
		if('show-sunriseset' in aValues && aValues['show-sunriseset']) {
			oTab.find('#'+sDevId+'_weather_values').append($('<div id="'+sDevId+'_sunset_icon" class="sunset_icon"></div><div class="sunset" id="'+sDevId+'_sunset"></div>'));
			oTab.find('#'+sDevId+'_weather_values').append($('<div id="'+sDevId+'_sunrise_icon" class="sunrise_icon"></div><div class="sunrise" id="'+sDevId+'_sunrise"></div>'));
			$('#'+sDevId+'_sunrise_icon').addClass('yellow');
			$('#'+sDevId+'_sunset_icon').addClass('gray');
		}
		if('show-illuminance' in aValues && aValues['show-illuminance']) {
			oTab.find('#'+sDevId+'_weather').append($('<div class="illuminance_icon"></div><div class="illuminance" id="'+sDevId+'_illu"></div>'));
		}
	}
	oTab.listview();
	oTab.listview("refresh");
}

function updateProcStatus(aValues) {
	if('ram' in aValues && 'cpu' in aValues) {
		var obj = $('#proc').text("CPU: "+aValues['cpu'].toFixed(2)+"% / RAM: "+aValues['ram'].toFixed(2)+"%");
	} else if('ram' in aValues) {
		var obj = $('#proc').text("RAM: "+aValues['ram'].toFixed(2)+"%");
	} else if('cpu' in aValues) {
		var obj = $('#proc').text("CPU: "+aValues['cpu'].toFixed(2)+"%");
	}
	obj.html(obj.html().replace(/\n/g,'<br/>'));
}

function createWebcamElement(sTabId, sDevId, aValues) {
	if($('#'+sDevId+'_webcam').length == 0) {
		if(bShowTabs) {
			oTab = $('#'+sTabId).find('ul');
		} else {
			oTab = $('#all');
		}
		if('name' in aValues) {
			oTab.append($('<li class="webcam" id="'+sDevId+'_webcam" data-icon="false" style="height: '+aValues['image-height']+'px;"><div class="name">'+aValues['name']+'</div></li>'));
		}
		oTab.find('#'+sDevId+'_webcam').append($('<div class="webcam_image" id="'+sDevId+'_image"><img id="'+sDevId+'_img" src="" height="'+aValues['image-height']+'"></div>'));
		if('image-width' in aValues && aValues['image-width'] > 0) {
			$('#'+sDevId+'_img').attr('width', aValues['image-width']+'px');
		}
	}
	oTab.listview();
	oTab.listview("refresh");
	if('poll-interval' in aValues) {
		window.setInterval(function() {
			var obj = $('#'+sDevId+'_img');
			if(!(sDevId in aWebcamUrl)) {
				aWebcamUrl[sDevId] = obj.attr('src');
			}
			if(aWebcamUrl[sDevId].indexOf('?') > -1) {
				obj.attr('src', aWebcamUrl[sDevId]+"&"+new Date().getTime());
			} else {
				obj.attr('src', aWebcamUrl[sDevId]+"?"+new Date().getTime());
			}
		}, aValues['poll-interval']*1000);
	}
}

function createXBMCElement(sTabId, sDevId, aValues) {
	if($('#'+sDevId+'_xbmc').length == 0) {
		if(bShowTabs) {
			oTab = $('#'+sTabId).find('ul');
		} else {
			oTab = $('#all');
		}
		if('name' in aValues) {
			oTab.append($('<li class="xbmc" id="'+sDevId+'_xbmc" data-icon="false"><div class="name">'+aValues['name']+'</div></li>'));
		}
		if('show-action' in aValues && aValues['show-action']) {
			oTab.find('#'+sDevId+'_xbmc').append($('<div class="action_icon" id="'+sDevId+'_action">&nbsp;</div>'));
			if('action' in aValues) {
				if(aValues['action'] == "play") {
					$('#'+sDevId+'_action').addClass('play');
				} else if(aValues['action'] == "pause") {
					$('#'+sDevId+'_action').addClass('pause');
				} else if(aValues['action'] == "stop") {
					$('#'+sDevId+'_action').addClass('stop');
				} else if(aValues['action'] == "active") {
					$('#'+sDevId+'_action').addClass('screen_active');
				} else if(aValues['action'] == "inactive") {
					$('#'+sDevId+'_action').addClass('screen_inactive');
				} else if(aValues['action'] == "shutdown") {
					$('#'+sDevId+'_action').addClass('shutdown');
				} else if(aValues['action'] == "home") {
					$('#'+sDevId+'_action').addClass('home');
				}
			}
		}
		if('show-media' in aValues && aValues['show-media']) {
			oTab.find('#'+sDevId+'_xbmc').append($('<div class="media_icon" id="'+sDevId+'_media">&nbsp;</div>'));
			if('media' in aValues) {
				if(aValues['media'] == "movie") {
					$('#'+sDevId+'_media').addClass('movie');
				} else if(aValues['media'] == "episode") {
					$('#'+sDevId+'_media').addClass('episode');
				} else if(aValues['media'] == "song") {
					$('#'+sDevId+'_media').addClass('song');
				}
			}
		}
	} else {
		if('show-action' in aValues && aValues['show-action']) {
			if($('#'+sDevId+'_action').attr("class").indexOf("play") != -1) {
				$('#'+sDevId+'_action').removeClass("play");
			}
			if($('#'+sDevId+'_action').attr("class").indexOf("pause") != -1) {
				$('#'+sDevId+'_action').removeClass("pause");
			}
			if($('#'+sDevId+'_action').attr("class").indexOf("stop") != -1) {
				$('#'+sDevId+'_action').removeClass("stop");
			}
			if($('#'+sDevId+'_action').attr("class").indexOf("screen_active") != -1) {
				$('#'+sDevId+'_action').removeClass("screen_active");
			}
			if($('#'+sDevId+'_action').attr("class").indexOf("screen_inactive") != -1) {
				$('#'+sDevId+'_action').removeClass("screen_inactive");
			}
			if($('#'+sDevId+'_action').attr("class").indexOf("shutdown") != -1) {
				$('#'+sDevId+'_action').removeClass("shutdown");
			}
			if($('#'+sDevId+'_action').attr("class").indexOf("home") != -1) {
				$('#'+sDevId+'_action').removeClass("home");
			}
			if(aValues['action'] == "play") {
				$('#'+sDevId+'_action').addClass('play');
			} else if(aValues['action'] == "pause") {
				$('#'+sDevId+'_action').addClass('pause');
			} else if(aValues['action'] == "stop") {
				$('#'+sDevId+'_action').addClass('stop');
			} else if(aValues['action'] == "active") {
				$('#'+sDevId+'_action').addClass('screen_active');
			} else if(aValues['action'] == "inactive") {
				$('#'+sDevId+'_action').addClass('screen_inactive');
			} else if(aValues['action'] == "shutdown") {
				$('#'+sDevId+'_action').addClass('shutdown');
			} else if(aValues['action'] == "home") {
				$('#'+sDevId+'_action').addClass('home');
			}
		}
		if('show-media' in aValues && aValues['show-media']) {
			if('media' in aValues) {
				if($('#'+sDevId+'_media').attr("class").indexOf("movie") != -1) {
					$('#'+sDevId+'_media').removeClass("movie");
				}
				if($('#'+sDevId+'_media').attr("class").indexOf("episode") != -1) {
					$('#'+sDevId+'_media').removeClass("episode");
				}
				if($('#'+sDevId+'_media').attr("class").indexOf("music") != -1) {
					$('#'+sDevId+'_media').removeClass("music");
				}
				if(aValues['media'] == "movie") {
					$('#'+sDevId+'_media').addClass('movie');
				} else if(aValues['media'] == "episode") {
					$('#'+sDevId+'_media').addClass('episode');
				} else if(aValues['media'] == "song") {
					$('#'+sDevId+'_media').addClass('music');
				}
			}
		}
	}
	oTab.listview();
	oTab.listview("refresh");
}

function createDateTimeElement(sTabId, sDevId, aValues) {
	aDateTime[sDevId] = new Array();

	if('format' in aValues) {
		sFormat = aValues['format'];
	} else {
		sFormat = sDateTimeFormat;
	}
	aDateTimeFormats[sDevId] = sFormat;

	if($('#'+sDevId+'_datetime').length == 0) {
		if(bShowTabs) {
			oTab = $('#'+sTabId).find('ul');
		} else {
			oTab = $('#all');
		}
		if('name' in aValues) {
			oTab.append($('<li id="'+sDevId+'_datetime" data-icon="false"><div class="name">'+aValues['name']+'</div></li>'));
			oTab.find('#'+sDevId+'_datetime').append($('<div id="'+sDevId+'_text" class="datetime"></div>'));
		}
	}
	oTab.listview();
	oTab.listview("refresh");
}

function updateVersions() {
	if(iPLVersion < iPLNVersion) {
		if(iFWVersion > 0) {
			var obj = $('#version').text("pilight v"+iPLVersion+" - "+language.available+" v"+iPLNVersion+" / filter firmware v"+iFWVersion);
		} else {
			var obj = $('#version').text("pilight v"+iPLVersion+" - "+language.available+" v"+iPLNVersion);
		}
	} else {
		if(iFWVersion > 0) {
			var obj = $('#version').text("pilight v"+iPLVersion+" / filter firmware  v"+iFWVersion);
		} else {
			var obj = $('#version').text("pilight v"+iPLVersion);
		}
	}
	obj.html(obj.html().replace(/\n/g,'<br/>'));
}

function createGUI(data) {
	$('#tabs').append($("<ul></ul>"));
	$.each(data['gui'], function(dindex, dvalues) {
		var lindex = dvalues['group'][0];
		if(oWebsocket && bStatsEnabled) {
			$('#proc').text("CPU: ...% / RAM: ...%");
		}
		if($('#'+alphaNum(lindex)).length == 0) {
			if(bShowTabs) {
				var oNavBar = $('#tabs');
				var oLi = $("<li></li>");
				var oA  = $('<a href="#'+alphaNum(lindex)+'"></a>');
				oA.text(lindex);
				oLi.append(oA);
				oNavBar.find("*").andSelf().each(function(){
					$(this).removeClass(function(i, cn){
						var matches = cn.match(/ui-[\w\-]+/g) || [];
						return (matches.join (' '));
					});
					if($(this).attr("class") == "") {
						$(this).removeAttr("class");
					}
				});
				oNavBar.navbar("destroy");
				oLi.appendTo($("#tabs ul"));
				oNavBar.navbar();
				$('#content').append($('<div class="content" id="'+alphaNum(lindex)+'"><ul data-role="listview" data-inset="true" data-theme="c"></ul></div>'));
			}
		}
		if(!bShowTabs) {
			if($('#heading_'+alphaNum(lindex)).length == 0) {
				if($('#all').length == 0) {
					$('#content').append($('<ul id="all" data-role="listview" data-inset="true" data-theme="c"><li data-role="list-divider" id="heading_'+alphaNum(lindex)+'" class="heading" role="heading">'+lindex+'</li></ul>'));
				} else {
					$('#all').append($('<li data-role="list-divider" id="heading_'+alphaNum(lindex)+'" class="heading" role="heading">'+lindex+'</li>'));
				}
			}
		}
		aValues = new Array();
		$.each(data['devices'][dindex], function(sindex, svalues) {
			aValues[sindex] = svalues;
		});
		$.each(dvalues, function(sindex, svalues) {
			aValues[sindex] = svalues;
		});
		if(aValues['type'] == 1 || aValues['type'] == 4) {
			createSwitchElement(alphaNum(lindex), dindex, aValues);
		} else if(aValues['type'] == 2) {
			createDimmerElement(alphaNum(lindex), dindex, aValues);
		} else if(aValues['type'] == 3) {
			createWeatherElement(alphaNum(lindex), dindex, aValues);
		} else if(aValues['type'] == 5) {
			createScreenElement(alphaNum(lindex), dindex, aValues);
		} else if(aValues['type'] == 7) {
			createPendingSwitchElement(alphaNum(lindex), dindex, aValues);
		} else if(aValues['type'] == 8) {
			createDateTimeElement(alphaNum(lindex), dindex, aValues);
		} else if(aValues['type'] == 9) {
			createXBMCElement(alphaNum(lindex), dindex, aValues);
		} else if(aValues['type'] == 11) {
			createWebcamElement(alphaNum(lindex), dindex, aValues);
		} else if(aValues['type'] == 15) {
			createLabelElement(alphaNum(lindex), dindex, aValues);
		}
		if(bShowTabs) {
			$(document).delegate('[data-role="navbar"] a', 'click', function(e) {
				var iPos = this.href.indexOf('#');
				var iLen = this.href.length;
				var sId = this.href.substring(iPos, iLen);

				$('#content .content').each(function() {
					if(this.id != sId.substring(1, sId.length) && $(this).css("display") != 'none') {
						$(this).toggle();
					}
				});

				if($(sId).css("display") == "none") {
					$(sId).toggle();
				}
				e.preventDefault();
			});
			if(!bInitialized) {
				$('#tabs a').each(function() {
					$(this).click();
					$(this).addClass("ui-btn-active");
					return false;
				});
			}
		}
	});
	$.mobile.loading("hide");
	bInitialized = true;
}

function parseValues(data) {
	if(data.hasOwnProperty("devices")) {
		var aValues = data.values;
		var iType = data.type;
		var aDevices = data['devices'];
		$.each(aDevices, function(dindex, dvalues) {
			$.each(aValues, function(vindex, vvalues) {
				if(iType == 1 || iType == 4) {
					if(vindex == 'state' && $('#'+dvalues+'_switch').length > 0) {
						if(vvalues == 'on' || vvalues == 'opened') {
							$('#'+dvalues+'_switch')[0].selectedIndex = 1;
						} else {
							$('#'+dvalues+'_switch')[0].selectedIndex = 0;
						}
						$('#'+dvalues+'_switch').slider('refresh');
					}
				} else if(iType == 2) {
					if(vindex == 'state' && $('#'+dvalues+'_switch').length > 0) {
						if(vvalues == 'on') {
							$('#'+dvalues+'_switch')[0].selectedIndex = 1;
						} else {
							$('#'+dvalues+'_switch')[0].selectedIndex = 0;
						}
						$('#'+dvalues+'_switch').slider('refresh');
					}
					if(vindex == 'dimlevel') {
						aDimLevel[dvalues] = vvalues;
						$('#'+dvalues+'_value').val(vvalues);
						$('#'+dvalues+'_dimmer').val(vvalues);
						$('#'+dvalues+'_dimmer').slider('refresh');
					}
				} else if(iType == 5) {
					if(vindex == 'state' && $('#'+dvalues+'_screen').length > 0) {
						if(vvalues == 'up') {
							$('#'+dvalues+'_screen_up').parent().find("label").addClass("ui-btn-active");
						} else {
							$('#'+dvalues+'_screen_down').parent().find("label").addClass("ui-btn-active");
						}
					}
				} else if(iType == 8) {
					if(dvalues in aDateTime && $('#'+dvalues+'_text').length > 0) {
						if(vvalues < 10) {
							vvalues = '0'+vvalues;
						}
						aDateTime[dvalues][vindex] = vvalues;
						aVal = aDateTime[dvalues];
						if('year' in aVal &&
						   'month' in aVal &&
						   'day' in aVal &&
						   'hour' in aVal &&
						   'minute' in aVal &&
						   'second' in aVal) {
							if(vindex == 'second') {
								sDate = moment(aVal['year']+'-'+aVal['month']+'-'+aVal['day']+' '+aVal['hour']+':'+aVal['minute']+':'+aVal['second'], ["YYYY-MM-DD HH:mm:ss"]).format(aDateTimeFormats[dvalues]);
								$('#'+dvalues+'_text').text(sDate);
							}
						}
					}
				} else if(iType == 7) {
					if(vindex == 'state' && $('#'+dvalues+'_pendingsw').length > 0) {
						if($('#'+dvalues+'_pendingsw').parent().attr("class").indexOf("ui-icon-loader") >= 0) {
							$('#'+dvalues+'_pendingsw').parent().removeClass('ui-icon-loader');
						}
						if($('#'+dvalues+'_pendingsw').parent().attr("class").indexOf("ui-icon-on") >= 0) {
							$('#'+dvalues+'_pendingsw').parent().removeClass('ui-icon-on');
						}
						if($('#'+dvalues+'_pendingsw').parent().attr("class").indexOf("ui-icon-off") >= 0) {
							$('#'+dvalues+'_pendingsw').parent().removeClass('ui-icon-off');
						}
						if(vvalues == 'running') {
							if(dvalues in aReadOnly && aReadOnly[dvalues] == 0) {
								$('#'+dvalues+'_pendingsw').button('enable');
							}
							aStates[dvalues] = "on";
							$('#'+dvalues+'_pendingsw').text(language.started);
							$('#'+dvalues+'_pendingsw').parent().addClass('ui-icon-on');
						} else if(vvalues == 'pending') {
							aStates[dvalues] = "pending";
							$('#'+dvalues+'_pendingsw').button('disable');
							$('#'+dvalues+'_pendingsw').text(language.toggling);
							$('#'+dvalues+'_pendingsw').parent().addClass('ui-icon-loader')
						} else {
							aStates[dvalues] = "off";
							$('#'+dvalues+'_pendingsw').button('enable');
							$('#'+dvalues+'_pendingsw').text(language.stopped);
							$('#'+dvalues+'_pendingsw').parent().addClass('ui-icon-off')
						}
						$('#'+dvalues+'_pendingsw').button('refresh');
					}
				} else if(iType == 3) {
					if(vindex == 'temperature' && $('#'+dvalues+'_temp').length > 0) {
						if(dvalues in aDecimals) {
							$('#'+dvalues+'_temp').text(vvalues.toFixed(aDecimals[dvalues]['temperature']));
						}
					} else if(vindex == 'humidity' && $('#'+dvalues+'_humi').length > 0) {
						if(dvalues in aDecimals) {
							$('#'+dvalues+'_humi').text(vvalues.toFixed(aDecimals[dvalues]['humidity']));
						}
					} else if(vindex == 'pressure' && $('#'+dvalues+'_pres').length > 0) {
						if(dvalues in aDecimals) {
							$('#'+dvalues+'_pres').text(vvalues.toFixed(aDecimals[dvalues]['pressure']));
						}
					} else if(vindex == 'rain' && $('#'+dvalues+'_rain').length > 0) {
						if(dvalues in aDecimals) {
							$('#'+dvalues+'_rain').text(vvalues.toFixed(aDecimals[dvalues]['rain']));
						}
					} else if(vindex == 'windgust' && $('#'+dvalues+'_windgust').length > 0) {
						if(dvalues in aDecimals) {
							$('#'+dvalues+'_windgust').text(vvalues.toFixed(aDecimals[dvalues]['wind']));
						}
					} else if(vindex == 'winddir' && $('#'+dvalues+'_winddir').length > 0) {
						$('#'+dvalues+'_weather .winddir_icon').css({transform: 'rotate(' + vvalues + 'deg)'});
					} else if(vindex == 'windavg' && $('#'+dvalues+'_windavg').length > 0) {
						if(dvalues in aDecimals) {
							$('#'+dvalues+'_windavg').text(vvalues.toFixed(aDecimals[dvalues]['wind']));
						}
					} else if(vindex == 'sunrise' && $('#'+dvalues+'_sunrise').length > 0) {
						if(dvalues in aDecimals) {
							$('#'+dvalues+'_sunrise').text(vvalues.toFixed(aDecimals[dvalues]['sunriseset']));
						}
					} else if(vindex == 'sunset' && $('#'+dvalues+'_sunset').length > 0) {
						if(dvalues in aDecimals) {
							$('#'+dvalues+'_sunset').text(vvalues.toFixed(aDecimals[dvalues]['sunriseset']));
						}
					} else if(vindex == 'illuminance' && $('#'+dvalues+'_illu').length > 0) {
						if(dvalues in aDecimals) {
							$('#'+dvalues+'_illu').text(vvalues.toFixed(aDecimals[dvalues]['illuminance']));
						}
					} else if(vindex == 'battery' && $('#'+dvalues+'_batt').length > 0) {
						if(vvalues == 1) {
							if($('#'+dvalues+'_batt').attr("class").indexOf("green") == -1) {
								$('#'+dvalues+'_batt').removeClass('red').addClass('green');
							}
						} else {
							if($('#'+dvalues+'_batt').attr("class").indexOf("red") == -1) {
								$('#'+dvalues+'_batt').removeClass('green').addClass('red');
							}
						}
					} else if(vindex == 'sun' && $('#'+dvalues+'_sunrise_icon').length > 0 && $('#'+dvalues+'_sunset_icon').length > 0) {
						if(vvalues == 'rise') {
							if($('#'+dvalues+'_sunrise_icon').attr("class").indexOf("yellow") == -1) {
								$('#'+dvalues+'_sunrise_icon').removeClass('gray').addClass('yellow');
							}
							if($('#'+dvalues+'_sunset_icon').attr("class").indexOf("gray") == -1) {
								$('#'+dvalues+'_sunset_icon').removeClass('blue').addClass('gray');
							}
						} else {
							if($('#'+dvalues+'_sunrise_icon').attr("class").indexOf("gray") == -1) {
								$('#'+dvalues+'_sunrise_icon').removeClass('yellow').addClass('gray');
							}
							if($('#'+dvalues+'_sunset_icon').attr("class").indexOf("blue") == -1) {
								$('#'+dvalues+'_sunset_icon').removeClass('gray').addClass('blue');
							}
						}
					} else if(vindex == 'update' && $('#'+dvalues+'_upd').length > 0) {
						if(vvalues == 0) {
							if($('#'+dvalues+'_upd').attr('class').indexOf('update_inactive') == -1) {
								$('#'+dvalues+'_upd').removeClass('update_active').addClass('update_inactive');
							}
						} else {
							if($('#'+dvalues+'_upd').attr('class').indexOf('update_active') == -1) {
								$('#'+dvalues+'_upd').removeClass('update_inactive').addClass('update_active');
							}
						}
					}
				} else if(iType == 9) {
					if(vindex == "action" && $('#'+dvalues+'_action').length > 0) {
						if($('#'+dvalues+'_action').attr("class").indexOf("play") != -1) {
							$('#'+dvalues+'_action').removeClass("play");
						}
						if($('#'+dvalues+'_action').attr("class").indexOf("pause") != -1) {
							$('#'+dvalues+'_action').removeClass("pause");
						}
						if($('#'+dvalues+'_action').attr("class").indexOf("stop") != -1) {
							$('#'+dvalues+'_action').removeClass("stop");
						}
						if($('#'+dvalues+'_action').attr("class").indexOf("screen_active") != -1) {
							$('#'+dvalues+'_action').removeClass("screen_active");
						}
						if($('#'+dvalues+'_action').attr("class").indexOf("screen_inactive") != -1) {
							$('#'+dvalues+'_action').removeClass("screen_inactive");
						}
						if($('#'+dvalues+'_action').attr("class").indexOf("shutdown") != -1) {
							$('#'+dvalues+'_action').removeClass("shutdown");
						}
						if($('#'+dvalues+'_action').attr("class").indexOf("home") != -1) {
							$('#'+dvalues+'_action').removeClass("home");
						}
						if(vvalues == "play") {
							$('#'+dvalues+'_action').addClass("play");
						} else if(vvalues == "pause") {
							$('#'+dvalues+'_action').addClass("pause");
						} else if(vvalues == "stop") {
							$('#'+dvalues+'_action').addClass("stop");
						} else if(vvalues == "active") {
							$('#'+dvalues+'_action').addClass("screen_active");
						} else if(vvalues == "inactive") {
							$('#'+dvalues+'_action').addClass("screen_inactive");
						} else if(vvalues == "shutdown") {
							$('#'+dvalues+'_action').addClass("shutdown");
						} else if(vvalues == "home") {
							$('#'+dvalues+'_action').addClass("home");
						}
					}
					if(vindex == "media" && $('#'+dvalues+'_media').length > 0) {
						if($('#'+dvalues+'_media').attr("class").indexOf("movie") != -1) {
							$('#'+dvalues+'_media').removeClass("movie");
						}
						if($('#'+dvalues+'_media').attr("class").indexOf("episode") != -1) {
							$('#'+dvalues+'_media').removeClass("episode");
						}
						if($('#'+dvalues+'_media').attr("class").indexOf("song") != -1) {
							$('#'+dvalues+'_media').removeClass("song");
						}
						if(vvalues == "movie") {
							$('#'+dvalues+'_media').addClass("movie");
						} else if(vvalues == "episode") {
							$('#'+dvalues+'_media').addClass("episode");
						} else if(vvalues == "song") {
							$('#'+dvalues+'_media').addClass("song");
						}
					}
				} else if(iType == 11) {
					if(vindex == 'url') {
						$('#'+dvalues+'_img').attr("src", vvalues);
					}
				} else if(iType == 15) {
					if(vindex == 'label') {
						$('#'+dvalues+' div.marquee .text').text(vvalues);
					} else if(vindex == 'color') {
						$('#'+dvalues+' div.marquee .text').css('color', vvalues);
					}
				}
			});
		});
	}
}

function parseData(data) {
	if(data.hasOwnProperty("config")) {
		config = data['config'];
		if(config.hasOwnProperty("gui") && config.hasOwnProperty("devices")) {
			createGUI(config);
			if('registry' in config && 'pilight' in config['registry']) {
				if('version' in config['registry']['pilight']) {
					if('current' in config['registry']['pilight']['version']) {
						iPLVersion = config['registry']['pilight']['version']['current'];
					}
					if('available' in config['registry']['pilight']['version']) {
						iNPLVersion = config['registry']['pilight']['version']['available'];
					}
				}
				updateVersions();
			}
		}
		if(oWebsocket) {
			oWebsocket.send("{\"action\":\"request values\"}");
		}
	} else if(data.hasOwnProperty("origin")) {
		if(data['origin'] == "update") {
			parseValues(data);
		} else if(data['origin'] == "core") {
			if(data['type'] == -1) {
				updateProcStatus(data['values']);
			}
		}
	} else if(data.hasOwnProperty("values")) {
		$.each(data['values'], function(dindex, dvalues) {
			parseValues(dvalues);
		});
	}
}

window.onbeforeunload = function() {
	if(oWebsocket) {
		oWebsocket.onclose = function() {}
		oWebsocket.close();
	}
}

function startAjax() {
	$.get(sHTTPProtocol+'://'+location.host+'/config?internal&'+$.now(), function(txt) {
		bConnected = true;
		if(!bSending) {
			var data = $.parseJSON(txt);
			parseData(data);
		}
	}).fail(function() {
		window.clearInterval(load);
		if(bConnected) {
			$.mobile.loading('show', {
				'text': language.connection_lost,
				'textVisible': true,
				'theme': 'b'
			});
			$('html').on({ 'touchstart mousedown' : function(){location.reload();}});
		} else {
			$.mobile.loading('show', {
				'text': language.connection_failed,
				'textVisible': true,
				'theme': 'b'
			});
			$('html').on({ 'touchstart mousedown' : function(){location.reload();}});
		}
	});
	var load = window.setInterval(function() {
		$.get(sHTTPProtocol+'://'+location.host+'/values?'+$.now(), function(txt) {
			bConnected = true;
			if(!bSending) {
				var data = $.parseJSON(txt);
				parseData(data);
			}
		}).fail(function() {
			window.clearInterval(load);
			if(bConnected) {
				$.mobile.loading('show', {
					'text': language.connection_lost,
					'textVisible': true,
					'theme': 'b'
				});
				$('html').on({ 'touchstart mousedown' : function(){location.reload();}});
			} else {
				$.mobile.loading('show', {
					'text': language.connection_failed,
					'textVisible': true,
					'theme': 'b'
				});
				$('html').on({ 'touchstart mousedown' : function(){location.reload();}});
			}
		});
	}, 1000);
}

function startWebsockets() {
	if(typeof MozWebSocket != "undefined") {
		oWebsocket = new MozWebSocket(sWSProtocol+"://"+location.host);
	} else if(typeof WebSocket != "undefined") {
		/* The characters after the trailing slash are needed for a wierd IE 10 bug */
		oWebsocket = new WebSocket(sWSProtocol+"://"+location.host+'/websocket');
	}
	if(oWebsocket) {
		oWebsocket.onopen = function(evt) {
			bConnected = true;
			oWebsocket.send("{\"action\":\"request config\"}");
		};
		oWebsocket.onclose = function(evt) {
			if(bConnected) {
				$.mobile.loading('show', {
					'text': language.connection_lost,
					'textVisible': true,
					'theme': 'b'
				});
				$('html').on({ 'touchstart mousedown' : function(){location.reload();}});
			} else {
				$.mobile.loading('show', {
					'text': language.connection_failed,
					'textVisible': true,
					'theme': 'b'
				});
				$('html').on({ 'touchstart mousedown' : function(){location.reload();}});
			}
		};
		oWebsocket.onerror = function(evt) {
			$.mobile.loading('show', {
				'text': language.unexpected_error,
				'textVisible': true,
				'theme': 'b'
			});
		};
		oWebsocket.onmessage = function(evt) {
			var data = $.parseJSON(evt.data);
			if(!('status' in data)) {
				parseData(data);
			}
		}
	}
}

$(document).ready(function() {
	if($('body').length == 1) {
		$.mobile.loading('show', {
			'text': language.connecting,
			'textVisible': true,
			'theme': 'b'
		});

		/* Use an AJAX request to check if the user want to enforce
		   an AJAX connection, or if he wants to use websockets */
		$.get(sHTTPProtocol+'://'+location.host+'/config?internal&'+$.now(), function(txt) {
			var data = $.parseJSON(JSON.stringify(txt));
			if('registry' in data) {
				if('webgui' in data['registry'] &&
					'tabs' in data['registry']['webgui']) {
					bShowTabs = data['registry']['webgui']['tabs'];
				}
	
				if(sHTTPProtocol == "https") {
					if('webserver' in data['registry'] &&
						 'ssl' in data['registry']['webserver'] &&
						 'certificate' in data['registry']['webserver']['ssl'] &&
						 'secure' in data['registry']['webserver']['ssl']['certificate']) {
					 if(data['registry']['webserver']['ssl']['certificate']['secure'] == 0) {
						 pemfile = 'pilight.pem';
						 if('location' in data['registry']['webserver']['ssl']['certificate']) {
							 pemfile = data['registry']['webserver']['ssl']['certificate']['location'];
						 }
						 alert(language['insecure_certificate'].format(pemfile));
						}
					}
				}
			}
			if('settings' in data && 'webgui-websockets' in data['settings']) {
				if(data['settings']['webgui-websockets'] == 0) {
					startAjax();
				} else {
					startWebsockets();
				}
			} else {
				startWebsockets();
			}
			if('settings' in data && 'stats-enable' in data['settings']) {
				bStatsEnabled = data['settings']['stats-enable'];
			}
		});

		$('div[data-role="header"] h1').css({'white-space':'normal'});
	}
});
