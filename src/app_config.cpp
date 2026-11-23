#include "app_config.h"

#include <ArduinoJson.h>

static uint8_t clampBrightness(long v) {
  if (v < kMinBrightness) return kMinBrightness;
  if (v > 255) return 255;
  return static_cast<uint8_t>(v);
}

bool AppConfig::load(fs::FS& fs, const char* path) {
  profiles.clear();
  error = "";

  File f = fs.open(path, "r");
  if (!f) {
    error = String(path) + " がありません";
    return false;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    error = String("JSON の解釈に失敗: ") + err.c_str();
    return false;
  }

  JsonVariantConst dev = doc["device"];
  device.brightness = clampBrightness(dev["brightness"] | 120L);
  device.qrBrightness = clampBrightness(dev["qr_brightness"] | 255L);
  device.dimAfterSec = dev["dim_after_sec"] | 20UL;
  device.offAfterSec = dev["off_after_sec"] | 30UL;
  device.standbyAfterMin = dev["standby_after_min"] | 5UL;
  device.qrTimeoutSec = dev["qr_timeout_sec"] | 60UL;
  device.wakeOnWrist = dev["wake_on_wrist"] | true;
  device.pixelShift = dev["pixel_shift"] | true;
  device.flip = dev["flip"] | true;
  device.eventModeEnabled = dev["event_mode"]["enabled"] | false;
  device.eventBrightness = clampBrightness(dev["event_mode"]["brightness"] | 40L);

  for (JsonObjectConst p : doc["profiles"].as<JsonArrayConst>()) {
    ProfileConfig pc;
    pc.id = p["id"] | "";
    pc.name = p["name"] | "";
    pc.avatar = p["avatar"] | "";
    pc.avatarSize = p["avatar_size"] | 280;
    for (JsonObjectConst l : p["links"].as<JsonArrayConst>()) {
      LinkConfig lc;
      lc.kind = l["kind"] | "web";
      lc.label = l["label"] | "";
      lc.url = l["url"] | "";
      if (lc.url.length() == 0) continue;
      pc.links.push_back(lc);
    }
    profiles.push_back(pc);
  }
  if (profiles.empty()) {
    error = "profiles が空です";
    return false;
  }
  return true;
}
