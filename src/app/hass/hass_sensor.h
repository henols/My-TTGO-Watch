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
#ifndef HASS_SENSOR_H
#define HASS_SENSOR_H

#include "config.h"
#include "hardware/alloc.h"
#include "WString.h"
#include "quickglui/widgets/label.h"
#include "quickglui/common/jsonconfig.h"

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

	void saveTo(JsonObject &dest) {
		dest["name"] = name;
		dest["topic"] = topic;
		dest["unit"] = unit;
		dest["device_class"] = device_class;
		dest["template"] = value_template;
	}
};

#endif

