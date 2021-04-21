/****************************************************************************
 *   2021-03-19
 *   Copyright  2021  Henrik Olsson
 *   Email: henols@gmail.com
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
#include <app/hass/hass_app.h>
#include "config.h"
#include <TTGO.h>
#include <WiFi.h>

#include "quickglui/quickglui.h"
#include "quickglui/app/page.h"

#include "gui/mainbar/mainbar.h"
#include "hardware/wifictl.h"
#include "gui/widget.h"
#include "gui/widget_styles.h"

#define CONFIG_PROP_PER_PAGE 5

// App icon must have an size of 64x64 pixel with an alpha channel
// Use https://lvgl.io/tools/imageconverter to convert your images and set "true color with alpha"
LV_IMG_DECLARE(hass_64px);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
lv_task_t *mainTaskHandle;

HassApp HassApp::app;

void HassApp::setup(void) {
	log_i("Starting Home Assisstant");

	// Load config and build user interface
	app.setUpConfig();
	int settingsPageCount = app.config.totalCount() / CONFIG_PROP_PER_PAGE + 1;

	log_i("Setting page count: %d, total count: %d", settingsPageCount, app.config.totalCount());

	app.useConfig(app.config, false);

	app.init("Home\nAssisstant", &hass_64px, 1, settingsPageCount);


	app.buildMain();


	mainTaskHandle = lv_task_create(mainTask, 250, LV_TASK_PRIO_MID, NULL);

	mqttClient.setCallback(mqttCallback);
	mqttClient.setBufferSize(512);

	wifictl_register_cb(WIFICTL_CONNECT_IP | WIFICTL_OFF_REQUEST | WIFICTL_OFF | WIFICTL_DISCONNECT, wifictlEventCb,
			"hass");
}

void HassApp::buildMain(void) {
	log_i("Building UI");

	Style hassDeskStyle;

	AppPage &main = Application::mainPage();

	// It also will auto-align child buttons on it:
	Container &desk = main.createChildContainer(LV_LAYOUT_PRETTY_MID);

	hassDeskStyle = Style::Create(ws_get_mainbar_style(), true);
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
}

void HassApp::onBuildSettingsSubPage(int id, lv_obj_t *tile) {

	int configStart = id * CONFIG_PROP_PER_PAGE;
	int configEnd = configStart + CONFIG_PROP_PER_PAGE;

	if (config.totalCount() - configStart < CONFIG_PROP_PER_PAGE) {
		configEnd = config.totalCount();
	}
	log_i("config page %d, start %d, end %d, tot %d", id, configStart, configEnd, config.totalCount());

	return;
	auto settingsTile = mainbar_get_tile_obj(settingsTileId() + id);

	Page page;
	page.assign(settingsTile);

	for (int i = configStart; i < configEnd; i++) {
		auto item = config.getOption(i);

		Widget line(page);
		line.size(LV_HOR_RES, 38);

		Label label(&line, item->name);
		label.alignInParentLeftMid(5, 0);

		switch (item->type()) {
		case OptionDataType::BoolOption: {
			auto option = (JsonBoolOption*) item;

			Switch switcher(&line, false);
			switcher.alignInParentRightMid(-5, 0);

			// Option value will be updated on applyFromUI() call
			option->assign(switcher);
			break;
		}
		case OptionDataType::StringOption: {
			auto option = (JsonStringOption*) item;

			TextArea editor(&line, "");
			editor.width(LV_HOR_RES / 2).alignInParentRightMid(-5, 0);

			// Option value will be updated on applyFromUI() call
			option->assign(editor);
			break;
		}
		}
	}

}

void HassApp::setUpConfig() {

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
	config.onLoadSaveHandler([this](JsonConfig &cfg) {
		bool widgetEnabled = cfg.getBoolean("widget"); // Is app widget enabled?
		if (widgetEnabled) {
			Application::icon().registerDesktopWidget("Hass", &hass_64px);
		} else {
			Application::icon().unregisterDesktopWidget();
		}
	});
}

template<typename T> T resolvePath(JsonVariantConst doc, String path) {
	log_i("Cutting down path %s", path.c_str());
	int first = path.indexOf('.');
	if (first <= 0) {
		return doc[path].as<T>();
	}
	String key = path.substring(0, first);
	if (!key.equals("value_json")) {
		doc = doc[key];
	}
	return resolvePath<T>(doc, path.substring(first + 1));
}

void HassApp::mqttCallback(char *topic, byte *payload, unsigned int length) {
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
		HassSensor *sensor = app.config.getByTopic(topic);
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

bool HassApp::wifictlEventCb(EventBits_t event, void *arg) {
	switch (event) {
	case WIFICTL_CONNECT_IP:
		log_i("Conneced to WIFI");
		if (app.autoconnect) {
			mqttClient.setServer(app.server.c_str(), app.port.toInt());
			if (!mqttClient.connect("hass", app.user.c_str(), app.password.c_str())) {
				log_e("connect to mqtt server %s failed", app.server.c_str());
				app.icon().showIndicator(ICON_INDICATOR_FAIL);
			} else {
				log_i("connect to mqtt server %s success", app.server.c_str());
				for (int i = 0; i < app.config.sensorCount(); i++) {
					log_i("mqtt subscribe to topic: %s", app.config.get(i)->topic.c_str());
					mqttClient.subscribe(app.config.get(i)->topic.c_str());
				}
				app.icon().showIndicator(ICON_INDICATOR_UPDATE);
			}
		}
		break;
	case WIFICTL_OFF_REQUEST:
	case WIFICTL_OFF:
	case WIFICTL_DISCONNECT:
		log_i("disconnect from mqtt server %s", app.server.c_str());
		mqttClient.disconnect();
		app.icon().hideIndicator();
		break;
	}
	return (true);
}

void HassApp::mainTask(lv_task_t *task) {
	mqttClient.loop();
}

