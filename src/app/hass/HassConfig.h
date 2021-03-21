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
#ifndef HASS_CONFIG_H
#define HASS_CONFIG_H

#include "config.h"
#include "quickglui/quickglui.h"
#include "HassSensor.h"

#define MAX_IR_BUTTONS 16

class HassConfig : public JsonConfig
{
public:
  HassConfig();

  HassSensor* add(const char* name);
  void del(const char* name);
  HassSensor* get(const char* name);
  HassSensor* getByTopic(const char* topic);

  HassSensor* get(int id) { return sensors[id]; }
  int sensorCount() { return count; }

public:
  int defBtnWidth = 95;
  int defBtnHeight = 33;
  int defSpacing = 3;

protected:
  virtual bool onSave(JsonDocument& document);
  virtual bool onLoad(JsonDocument& document);
  virtual size_t getJsonBufferSize() { return 16000; }

protected:
  HassSensor *sensors[MAX_IR_BUTTONS];
  int count = 0;
};

#endif

