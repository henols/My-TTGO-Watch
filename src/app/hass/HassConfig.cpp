#include <app/hass/HassConfig.h>
#include "hardware/alloc.h"

HassConfig::HassConfig() :
		JsonConfig("home-assisstant.json") {
	count = 0;
	// This file is too big for that!
	prettyJson = false;
}

HassSensor* HassConfig::add(const char *name) {
	HassSensor *sensor = get(name);
	if (sensor == nullptr) {
		log_i("Adding sensor at %d by name: %s", count, name);
		void *pointer = MALLOC(sizeof(HassSensor));
		sensor = new (pointer) HassSensor();
		sensor->name = name;
		sensor->pos = count;
		sensors[count++] = sensor;
	}
	return sensor;
}

void HassConfig::del(const char *name) {
	log_i("Deleting sensor by name: %s", name);
	bool found = false;
	for (int i = 0; i < count; i++) {
		if (strcmp(sensors[i]->name.c_str(), name) == 0) {
			if (sensors[i]->uiLabel.isCreated()) {
				sensors[i]->uiLabel.free();
			}
			delete sensors[i];
			found = true;
		}
		if (found){
			sensors[i] = sensors[i + 1];
			sensors[i]->pos = i;
		}
	}
	if (found)
		count--;
}

HassSensor* HassConfig::get(const char *name) {
	log_i("Get sensor by name: %s", name);
	for (int i = 0; i < count; i++) {
		if (strcmp(sensors[i]->name.c_str(), name) == 0) {
			return sensors[i];
		}
	}
	return nullptr;
}

HassSensor* HassConfig::getByTopic(const char *topic) {
	log_i("Get sensor by topic: %s", topic);
	for (int i = 0; i < count; i++) {
		if (strcmp(sensors[i]->topic.c_str(), topic) == 0) {
			return sensors[i];
		}
	}
	return nullptr;
}

bool HassConfig::onSave(JsonDocument &document) {

	log_i("Saving config, sensors: %d", count);
	auto pagesArray = document.createNestedArray("sensors");
	for (int i = 0; i < count; i++) {
		log_i("Saving sensor: %s, pos: %d", sensors[i]->name, i);
		JsonObject sensorRec = pagesArray.createNestedObject();
		sensorRec["name"] = sensors[i]->name;
		sensorRec["topic"] = sensors[i]->topic;
		sensorRec["unit"] = sensors[i]->unit;
		sensorRec["device_class"] = sensors[i]->device_class;
		sensorRec["template"] = sensors[i]->value_template;
	}
	return JsonConfig::onSave(document);
}

bool HassConfig::onLoad(JsonDocument &document) {
	log_i("onLoad this");

	JsonArray sensors = document["sensors"].as<JsonArray>();
	if (!sensors.isNull()) {
		for (JsonVariant record : sensors) {
			JsonObject configuration = record.as<JsonObject>();
			log_i("Loading sensor: %s", (const char* ) configuration["name"]);
			auto sensor = add(configuration["name"]);
			sensor->loadFrom(configuration);
		}
	}
	log_i("onLoad super");
	return JsonConfig::onLoad(document);
}
