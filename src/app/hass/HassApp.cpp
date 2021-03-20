/****************************************************************************
 *   Aug 3 12:17:11 2020
 *   Copyright  2020  Dirk Brosswick
 *   Email: dirk.brosswick@googlemail.com
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <app/hass/HassApp.h>
#include "config.h"
#include <TTGO.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "quickglui/quickglui.h"

#include "gui/mainbar/mainbar.h"
#include "hardware/blectl.h"
#include "hardware/wifictl.h"
#include "gui/widget.h"
#include "gui/app.h"

#include "HassConfig.h"

// App icon must have an size of 64x64 pixel with an alpha channel
// Use https://lvgl.io/tools/imageconverter to convert your images and set "true color with alpha"
LV_IMG_DECLARE(hass_64px);

HassConfig config;
Application app;

Style hassDeskStyle;
WiFiClient HassApp_wifiClient;
PubSubClient HassApp_mqtt_client(HassApp_wifiClient);

lv_task_t *_HassApp_main_task;

bool HassApp_wifictl_event_cb(EventBits_t event, void *arg);
void HassApp_main_task(lv_task_t *task);
void HassApp_mqtt_callback(char *topic, byte *payload, unsigned int length);

String server;
String port;
bool ssl = false;
String user;
String password;
String topic;
bool autoconnect = false;
bool widget = false;

/*
 * setup routine for IR Controller app
 */
void HassApp_setup(void) {
	log_i("Starting Home Assisstant");
	app.init("Home\nAssisstant", &hass_64px, 1, 3);

	// Load config and build user interface
	HassApp_build_settings();

	HassApp_build_main();

	wifictl_register_cb(WIFICTL_CONNECT_IP | WIFICTL_OFF_REQUEST | WIFICTL_OFF | WIFICTL_DISCONNECT,
			HassApp_wifictl_event_cb, "hass");

	_HassApp_main_task = lv_task_create(HassApp_main_task, 250, LV_TASK_PRIO_MID, NULL);

	HassApp_mqtt_client.setCallback(HassApp_mqtt_callback);
	HassApp_mqtt_client.setBufferSize(512);

//    blectl_register_cb(BLECTL_MSG, HassApp_bluetooth_event_cb, "Home Assisstant setup");
}

void HassApp_build_main(void) {
	log_i("Building UI");
//    if (settingsAction == HassAppSettingsAction::Load)
	// hassConfig.load();

	AppPage &main = app.mainPage();
	// Create parent widget which will contains all IR control buttons
	// It also will auto-align child buttons on it:
	Container &desk = main.createChildContainer(LV_LAYOUT_PRETTY_MID);

	hassDeskStyle = Style::Create(mainbar_get_style(), true);
	hassDeskStyle.paddingInner(config.defSpacing);
	hassDeskStyle.padding(7, 16, 7, 16);
	desk.style(hassDeskStyle);

	for (int i = 0; i < config.sensorCount(); i++) {
		auto labelConfig = config.get(i);
		log_i("Adding label nr: %d, name: %s", i, labelConfig->name);
		if (labelConfig->uiLabel.isCreated()) {
			labelConfig->uiLabel.alignInParentTopLeft(0, 0); // Call auto alignment
			continue;
		}

		// Add new button
		Label label(&desk, labelConfig->name.c_str());
		label.size(config.defBtnWidth, config.defBtnHeight);
		labelConfig->uiLabel = label;
	}

	// Refresh screen
	lv_obj_invalidate(lv_scr_act());

//    if (settingsAction == HassAppSettingsAction::Save)
//    	hassConfig.save();
}

void HassApp_build_settings() {

	log_i("Build settings");
	// Create full options list and attach items to variables
	config.addString("server", 32).assign(&server).setDigitsMode(false, NULL);
	config.addString("port", 5, "1883").setDigitsMode(true, "0123456789").assign(&port);
	config.addString("topic", 48, "homeassistant/sensor/#").assign(&topic);
	config.addBoolean("widget", false).assign(&widget);
	config.addBoolean("autoconnect", false).assign(&autoconnect);
	config.addBoolean("ssl", false).assign(&ssl);
	config.addString("user", 12).assign(&user);
	config.addString("password", 12).assign(&password);

	// Switch desktop widget state based on the cuurent settings when changed
	config.onLoadSaveHandler([](JsonConfig &cfg) {
		bool widgetEnabled = cfg.getBoolean("widget"); // Is app widget enabled?
		if (widgetEnabled) {
			app.icon().registerDesktopWidget("Hass", &hass_64px);
		} else {
			app.icon().unregisterDesktopWidget();
		}
	});

	log_i("Use config");
	app.useConfig(config, true); // true - auto create settings page widgets
	log_i("retrun");
}

JsonVariantConst value(JsonVariantConst variant, String key) {
	return variant[key];
}

template<typename T>
T resolvePath(JsonVariantConst doc, String path) {
	log_i("Cutting down path %s", path.c_str());
	int first = path.indexOf('.');
	if (first <= 0) {
		return value(doc, path).as<T>();
	}
	String key = path.substring(0, first);
	if (!key.equals("value_json")) {
		doc = doc[key];
	}
	return resolvePath<T>(doc, path.substring(first + 1));
}

void HassApp_mqtt_callback(char *topic, byte *payload, unsigned int length) {
	char *msg = NULL;
	msg = (char*) CALLOC(length + 1, 1);
	if (msg == NULL) {
		log_e("calloc failed");
		return;
	}
	memcpy(msg, payload, length);
	log_i("MQTT topic: %s", topic);
	SpiRamJsonDocument doc(strlen(msg) * 2);

	DeserializationError error = deserializeJson(doc, msg);
	if (error) {
		log_e("hass message deserializeJson() failed: %s", error.c_str());
	} else {
		HassSensor *sensor = config.getByTopic(topic);
		if (sensor != nullptr) {
			auto templ = String(sensor->value_template);
			templ.trim();
			if (templ.startsWith("{{") && templ.endsWith("}}")) {
				templ = templ.substring(2, templ.length() - 2);
				templ.trim();
				log_i("Payload template: %s", templ.c_str());
				String msgValue = resolvePath<String>(doc.as<JsonVariantConst>(), templ);
				log_i("Set payload to sensor %s: %s%s", sensor->name, msgValue, sensor->unit);
				app.icon().showIndicator(ICON_INDICATOR_OK);
				sensor->uiLabel.text(sensor->name + ": " + msgValue + sensor->unit);
				if (sensor->pos == 0) {
					app.icon().widgetText(msgValue + sensor->unit);
				}
				lv_obj_invalidate(lv_scr_act());
			}
		}
	}
	doc.clear();
	free(msg);
}

bool HassApp_wifictl_event_cb(EventBits_t event, void *arg) {
	switch (event) {
	case WIFICTL_CONNECT_IP:
		log_i("Conneced to WIFI");
		if (autoconnect) {
			HassApp_mqtt_client.setServer(server.c_str(), port.toInt());
			if (!HassApp_mqtt_client.connect("hass", user.c_str(), password.c_str())) {
				log_e("connect to mqtt server %s failed", server.c_str());
				app.icon().showIndicator(ICON_INDICATOR_FAIL);
			} else {
				log_i("connect to mqtt server %s success", server.c_str());
				for (int i = 0; i < config.sensorCount(); i++) {
					log_i("mqtt subscribe to topic: %s", config.get(i)->topic.c_str());
					HassApp_mqtt_client.subscribe(config.get(i)->topic.c_str());
				}
				app.icon().showIndicator(ICON_INDICATOR_UPDATE);
			}
		}
		break;
	case WIFICTL_OFF_REQUEST:
	case WIFICTL_OFF:
	case WIFICTL_DISCONNECT:
		log_i("disconnect from mqtt server %s", server.c_str());
		HassApp_mqtt_client.disconnect();
		app.icon().hideIndicator();
		break;
	}
	return (true);
}

void HassApp_main_task(lv_task_t *task) {
	HassApp_mqtt_client.loop();
}

