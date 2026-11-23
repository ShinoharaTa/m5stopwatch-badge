#include "settings_store.h"

#include <Preferences.h>
#include <esp32-hal-log.h>

namespace {
constexpr const char* kNs = "badge";
}

void SettingsStore::load(DeviceConfig& dev) {
  Preferences p;
  if (!p.begin(kNs, false)) return;  // 読み書きで開く(初回は名前空間を作るだけ)
  if (p.isKey("bri")) dev.brightness = p.getUChar("bri", dev.brightness);
  if (p.isKey("ebri")) dev.eventBrightness = p.getUChar("ebri", dev.eventBrightness);
  if (p.isKey("dim")) dev.dimAfterSec = p.getUInt("dim", dev.dimAfterSec);
  if (p.isKey("off")) dev.offAfterSec = p.getUInt("off", dev.offAfterSec);
  if (p.isKey("stby")) dev.standbyAfterMin = p.getUInt("stby", dev.standbyAfterMin);
  if (p.isKey("evt")) dev.eventModeEnabled = p.getBool("evt", dev.eventModeEnabled);
  if (p.isKey("pshift")) dev.pixelShift = p.getBool("pshift", dev.pixelShift);
  if (p.isKey("flip")) dev.flip = p.getBool("flip", dev.flip);
  p.end();
  // 以前の版で 0 が保存されていることがあるので下限で丸める
  if (dev.brightness < kMinBrightness) dev.brightness = kMinBrightness;
  if (dev.eventBrightness < kMinBrightness) dev.eventBrightness = kMinBrightness;
  log_i("[store] 読み込み bri=%u ebri=%u dim=%lu off=%lu evt=%d pshift=%d flip=%d", dev.brightness, dev.eventBrightness,
        static_cast<unsigned long>(dev.dimAfterSec), static_cast<unsigned long>(dev.offAfterSec),
        dev.eventModeEnabled ? 1 : 0, dev.pixelShift ? 1 : 0, dev.flip ? 1 : 0);
}

void SettingsStore::save(const DeviceConfig& dev, const DeviceConfig& defaults) {
  Preferences p;
  if (!p.begin(kNs, false)) {
    log_e("[store] NVS を開けない");
    return;
  }
  auto u8 = [&p](const char* k, uint8_t v, uint8_t d) {
    if (v != d) p.putUChar(k, v);
    else if (p.isKey(k)) p.remove(k);
  };
  auto u32 = [&p](const char* k, uint32_t v, uint32_t d) {
    if (v != d) p.putUInt(k, v);
    else if (p.isKey(k)) p.remove(k);
  };
  auto b = [&p](const char* k, bool v, bool d) {
    if (v != d) p.putBool(k, v);
    else if (p.isKey(k)) p.remove(k);
  };
  u8("bri", dev.brightness, defaults.brightness);
  u8("ebri", dev.eventBrightness, defaults.eventBrightness);
  u32("dim", dev.dimAfterSec, defaults.dimAfterSec);
  u32("off", dev.offAfterSec, defaults.offAfterSec);
  u32("stby", dev.standbyAfterMin, defaults.standbyAfterMin);
  b("evt", dev.eventModeEnabled, defaults.eventModeEnabled);
  b("pshift", dev.pixelShift, defaults.pixelShift);
  b("flip", dev.flip, defaults.flip);
  p.end();
  log_i("[store] 保存");
}

void SettingsStore::clear() {
  Preferences p;
  if (p.begin(kNs, false)) {
    p.clear();
    p.end();
  }
  log_i("[store] 初期値に戻した");
}
