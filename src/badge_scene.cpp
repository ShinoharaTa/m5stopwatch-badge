#include "badge_scene.h"

#include <esp_random.h>

#include "input.h"
#include "power_manager.h"
#include "settings_store.h"
#include "ui.h"

namespace {
constexpr uint32_t kShiftIntervalMs = 60 * 1000;
constexpr int kShiftRange = 3;
constexpr uint32_t kBatteryShowMs = 2000;
constexpr uint32_t kRelockMs = 15 * 1000;

int randomShift() { return static_cast<int>(esp_random() % (2 * kShiftRange + 1)) - kShiftRange; }
}  // namespace

void BadgeScene::toast(const char* s, uint32_t ms) {
  _toast = s;
  _toastUntil = millis() + ms;
}

void BadgeScene::enter(AppContext& ctx) {
  const uint32_t now = millis();
  _lastShift = now;
  _dialog = false;
  if (ctx.justWoke) {
    ctx.justWoke = false;
    _batteryUntil = now + kBatteryShowMs;
  } else {
    _batteryUntil = 0;
  }
  render(ctx, now);
}

void BadgeScene::render(AppContext& ctx, uint32_t now) {
  using namespace ui;
  const auto& p = ctx.profile();
  const bool eventMode = ctx.cfg->device.eventModeEnabled;
  const bool locked = eventMode && ctx.locked;
  const bool showBattery = _batteryUntil != 0 && static_cast<int32_t>(_batteryUntil - now) > 0;
  const bool showToast = _toastUntil != 0 && static_cast<int32_t>(_toastUntil - now) > 0;

  clear();
  auto& f = frame();

  // アバター(中心に置く)。JPEG の四隅は黒なので、輪より先に描く
  const int s = p.avatarSize;
  bool drawn = false;
  if (p.avatar.length() > 0) {
    drawn = f.drawJpgFile(LittleFS, p.avatar.c_str(), kCx - s / 2, kCy - s / 2);
    if (!drawn) log_w("[badge] アバターを描けない: %s", p.avatar.c_str());
  }
  if (!drawn) {
    f.fillCircle(kCx, kCy, s / 2, rgb(40, 40, 40));
    text(kindGlyph("", p.name), kCx, kCy, s / 3, kText);
  }

  // 外輪(復帰直後は電池残量の円弧に置き換える)。常時表示中は焼き付きを避けて出さない
  if (showBattery) {
    const int pct = batteryPercent();
    ring(158, 6, kTrack);
    if (pct > 0) arc(155, 161, 0, 360.0f * pct / 100.0f, pct < 20 ? kBad : kGood);
    text(String(pct) + "%", kCx, 62, 20, pct < 20 ? kBad : kGood);
  } else if (!eventMode) {
    ring(152, 2, kTrack);
  }

  // プロファイル位置: 12 時付近に 14° の円弧をプロファイル数だけ(間隔 4°)
  const int n = static_cast<int>(ctx.cfg->profiles.size());
  if (n > 1 && !locked && !showBattery && !showToast) {
    const float span = n * 14.0f + (n - 1) * 4.0f;
    float a = -span / 2;
    for (int i = 0; i < n; ++i) {
      arc(202, 207, a, a + 14.0f, i == ctx.profileIndex ? kText : kTextDisabled);
      a += 18.0f;
    }
  }

  text(fit(p.name, 250, 28), kCx, 408, 28, kText);  // 250px: 首掛け向きのボタン表示の円弧に掛からない幅
  if (showToast && !showBattery) text(_toast, kCx, 62, 20, kTextSub);
  if (!locked && !_dialog) buttonHints("リンク", "アプリ", 0.55f);

  if (_dialog) {
    dialog(eventMode ? "イベントモードを OFF にする?" : "イベントモードを ON にする?", "はい", "いいえ");
  }
  present(_dx, _dy);
}

SceneId BadgeScene::tick(AppContext& ctx, uint32_t now) {
  bool redraw = false;
  if (ctx.cfg->device.pixelShift && ui::elapsedMs(now, _lastShift) >= kShiftIntervalMs) {
    _lastShift = now;
    _dx = randomShift();
    _dy = randomShift();
    redraw = true;
  }
  if (_batteryUntil != 0 && static_cast<int32_t>(_batteryUntil - now) <= 0) {
    _batteryUntil = 0;
    redraw = true;
  }
  if (_toastUntil != 0 && static_cast<int32_t>(_toastUntil - now) <= 0) {
    _toastUntil = 0;
    redraw = true;
  }
  if (_dialog && ctx.power && ctx.power->idleMs(now) >= kRelockMs) {
    _dialog = false;
    redraw = true;
  }
  if (ctx.cfg->device.eventModeEnabled && !ctx.locked && !_dialog && ctx.power &&
      ctx.power->idleMs(now) >= kRelockMs) {
    ctx.locked = true;
    log_i("[badge] ロック");
    redraw = true;
  }
  if (redraw) render(ctx, now);
  return SceneId::Same;
}

SceneId BadgeScene::onInput(AppContext& ctx, const InputEvent& ev) {
  const uint32_t now = millis();
  auto& dev = ctx.cfg->device;
  const int n = static_cast<int>(ctx.cfg->profiles.size());

  if (_dialog) {
    if (ev.type == InputEvent::KeyAClick) {
      dev.eventModeEnabled = !dev.eventModeEnabled;
      ctx.locked = dev.eventModeEnabled;
      if (ctx.store && ctx.defaults) ctx.store->save(dev, *ctx.defaults);
      if (ctx.power) ctx.power->refresh();
      log_i("[badge] イベントモード %s", dev.eventModeEnabled ? "ON" : "OFF");
      _dialog = false;
      if (dev.eventModeEnabled) toast("ロック中。A+B 同時押しで解除", 2500);
      render(ctx, now);
    } else if (ev.type == InputEvent::KeyBClick || ev.type == InputEvent::Tap) {
      _dialog = false;
      render(ctx, now);
    }
    return SceneId::Same;
  }

  // 常時表示中のロック: A+B 同時押しだけ受け付ける
  if (dev.eventModeEnabled && ctx.locked) {
    switch (ev.type) {
      case InputEvent::BothPress:
        ctx.locked = false;
        input::cancelComboHome();
        log_i("[badge] ロック解除");
        toast("ロック解除", 1200);
        render(ctx, now);
        break;
      case InputEvent::Tap:
      case InputEvent::TouchHold:
      case InputEvent::SwipeLeft:
      case InputEvent::SwipeRight:
      case InputEvent::KeyAClick:
      case InputEvent::KeyBClick:
      case InputEvent::KeyAHold:
      case InputEvent::KeyBHold:
        toast("A+B 同時押しで解除", 1500);
        render(ctx, now);
        break;
      default:
        break;
    }
    return SceneId::Same;
  }

  switch (ev.type) {
    case InputEvent::SwipeLeft:
      ctx.profileIndex = (ctx.profileIndex + 1) % n;
      ctx.linkIndex = 0;
      render(ctx, now);
      break;
    case InputEvent::SwipeRight:
      ctx.profileIndex = (ctx.profileIndex + n - 1) % n;
      ctx.linkIndex = 0;
      render(ctx, now);
      break;
    case InputEvent::Tap:
    case InputEvent::KeyAClick:
      if (!ctx.profile().links.empty()) return SceneId::Links;
      break;
    case InputEvent::KeyBClick:
      return SceneId::Launcher;
    case InputEvent::TouchHold:
      _dialog = true;
      render(ctx, now);
      break;
    case InputEvent::KeyAHold:
      dev.flip = !dev.flip;
      if (ctx.store && ctx.defaults) ctx.store->save(dev, *ctx.defaults);
      ui::applyFlip(dev.flip);
      toast(dev.flip ? "首掛けの向き" : "手持ちの向き", 1500);
      render(ctx, now);
      break;
    case InputEvent::KeyBHold:
      if (ctx.power) ctx.power->screenOff(now);
      break;
    default:
      break;
  }
  return SceneId::Same;
}
