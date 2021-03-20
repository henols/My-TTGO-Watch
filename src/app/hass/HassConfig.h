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

