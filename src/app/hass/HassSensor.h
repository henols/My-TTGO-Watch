#ifndef HASS_SENSOR_H
#define HASS_SENSOR_H

#include "config.h"
#include "hardware/alloc.h"
#include "WString.h"

#define RAW_CODE_BUFER_SIZE sizeof(uint16_t)*120

struct HassSensor {
	~HassSensor() {
	}
	String name;
	String topic;
	String unit;
	String device_class;
	String value_template;
	Label uiLabel;
	int pos;

	void resize(int size) {
	}

	void loadFrom(JsonObject &source) {

		if (source.containsKey("name")) {
			const char *renamed = source["name"];
			if (name != renamed && uiLabel.isCreated()) {
				uiLabel.text(renamed);
			}
			name = renamed;
		}

		if (source.containsKey("topic")) {
			topic = (const char*) source["topic"];
		}
		if (source.containsKey("unit")) {
			unit = (const char*) source["unit"];
		}
		if (source.containsKey("template")) {
			value_template = (const char*) source["template"];
		}
		if (source.containsKey("device_class")) {
			device_class = (const char*) source["device_class"];
		}
	}
};

#endif

