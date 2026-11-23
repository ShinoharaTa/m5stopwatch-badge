#include "power_manager.h"

#include <algorithm>

#include "standby.h"
#include "ui.h"

namespace {
constexpr uint8_t kMenuMinBrightness = 80;
constexpr uint32_t kMenuIdleMinSec = 60;
}  // namespace

void PowerManager::begin(DeviceConfig* dev) {
  _dev = dev;
  _lastActivity = millis();
  _state = ScreenState::On;
  _policy = ScreenPolicy::Badge;
  applyBrightness();
}

void PowerManager::setPolicy(ScreenPolicy p) {
  if (!_dev) return;
  _policy = p;
  if (p == ScreenPolicy::Menu) _menuBrightness = std::max(_dev->brightness, kMenuMinBrightness);
  _lastActivity = millis();
  if (_state == ScreenState::Dim) _state = ScreenState::On;
  if (_state != ScreenState::Off) applyBrightness();
}

void PowerManager::refresh() {
  if (_state != ScreenState::Off) applyBrightness();
}

uint8_t PowerManager::target() const {
  if (_state == ScreenState::Off || !_dev) return 0;
  switch (_policy) {
    case ScreenPolicy::Qr: return _dev->qrBrightness;
    case ScreenPolicy::Menu: return _menuBrightness;
    case ScreenPolicy::Badge:
    default: {
      const uint8_t base = _dev->eventModeEnabled ? _dev->eventBrightness : _dev->brightness;
      if (_state == ScreenState::Dim) return std::max<uint8_t>(kMinBrightness, base / 4);
      return base;
    }
  }
}

void PowerManager::applyBrightness() { M5.Display.setBrightness(target()); }

void PowerManager::setState(ScreenState s, uint32_t now) {
  if (s == _state) return;
  const ScreenState prev = _state;
  _state = s;
  if (s == ScreenState::Off) {
    M5.Display.sleep();  // 輝度 0 にしてパネルをスリープ
    _offSince = now;
    log_i("[power] screen off");
    return;
  }
  if (prev == ScreenState::Off) {
    M5.Display.wakeup();
    log_i("[power] screen on");
  } else if (s == ScreenState::Dim) {
    log_i("[power] dim");
  }
  applyBrightness();
}

PowerManager::Wake PowerManager::noteActivity(uint32_t now) {
  _lastActivity = now;
  const ScreenState prev = _state;
  setState(ScreenState::On, now);
  if (prev == ScreenState::Off) return Wake::FromOff;
  if (prev == ScreenState::Dim) return Wake::FromDim;
  return Wake::None;
}

void PowerManager::screenOff(uint32_t now) {
  _lastActivity = now;
  setState(ScreenState::Off, now);
}

uint32_t PowerManager::idleMs(uint32_t now) const { return ui::elapsedMs(now, _lastActivity); }

PowerManager::Action PowerManager::tick(uint32_t now) {
  if (!_dev) return Action::None;
  if (_state == ScreenState::Off) {
    if (_dev->standbyAfterMin > 0 && ui::elapsedMs(now, _offSince) >= _dev->standbyAfterMin * 60000UL) {
      standby::enter();  // 戻らない
    }
    return Action::None;
  }
  const uint32_t idle = idleMs(now);
  switch (_policy) {
    case ScreenPolicy::Qr:
      return Action::None;
    case ScreenPolicy::Menu: {
      const uint32_t limit = std::max<uint32_t>(_dev->offAfterSec, kMenuIdleMinSec) * 1000UL;
      if (idle >= limit) {
        if (_dev->eventModeEnabled) return Action::GoBadge;
        setState(ScreenState::Off, now);
      }
      return Action::None;
    }
    case ScreenPolicy::Badge:
    default:
      if (_dev->eventModeEnabled) {
        if (_state == ScreenState::Dim) setState(ScreenState::On, now);
        return Action::None;
      }
      if (_dev->offAfterSec > 0 && idle >= _dev->offAfterSec * 1000UL) {
        setState(ScreenState::Off, now);
      } else if (_state == ScreenState::On && _dev->dimAfterSec > 0 && idle >= _dev->dimAfterSec * 1000UL) {
        setState(ScreenState::Dim, now);
      }
      return Action::None;
  }
}
