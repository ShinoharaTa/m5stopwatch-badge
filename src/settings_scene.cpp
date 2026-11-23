#include "settings_scene.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "power_manager.h"
#include "settings_store.h"
#include "ui.h"

namespace {
constexpr int kTop = 90;       // 一覧の表示範囲
constexpr int kBottom = 402;
constexpr int kLeft = 96;      // 行の左右
constexpr int kRight = 376;
constexpr int kTrackX0 = 104;  // シークバーの左右
constexpr int kTrackX1 = 364;
constexpr int kSegW = 136;
constexpr int kSegH = 34;
constexpr int kSwitchW = 56;
constexpr int kSwitchH = 30;
}  // namespace

void SettingsScene::enter(AppContext& ctx) {
  if (_rows.empty()) {
    _rows = {
        {Field::Flip, Kind::Segment, "画面の向き", 0, 1, 1, 60},
        {Field::Brightness, Kind::Slider, "輝度", kMinBrightness, 255, 5, 84},
        {Field::Event, Kind::Toggle, "イベントモード", 0, 1, 1, 60},
        {Field::EventBrightness, Kind::Slider, "イベント時の輝度", kMinBrightness, 255, 5, 84},
        {Field::PixelShift, Kind::Toggle, "ピクセルシフト", 0, 1, 1, 60},
        {Field::Dim, Kind::Slider, "減光まで", 0, 120, 5, 84},
        {Field::Off, Kind::Slider, "消灯まで", 0, 300, 10, 84},
        {Field::Standby, Kind::Slider, "スタンバイまで", 0, 60, 1, 84},
        {Field::Reset, Kind::Action, "初期値に戻す", 0, 0, 0, 60},
        {Field::Battery, Kind::Info, "電池", 0, 0, 0, 52},
        {Field::Version, Kind::Info, "バージョン", 0, 0, 0, 52},
    };
    _contentH = 0;
    for (const auto& r : _rows) _contentH += r.h;
  }
  _activeSlider = -1;
  _scrolling = false;
  _dirty = false;
  _scroll = std::min(std::max(_scroll, 0.0f), maxScroll());
  render(ctx);
}

void SettingsScene::leave(AppContext& ctx) { commit(ctx); }

void SettingsScene::commit(AppContext& ctx) {
  if (_dirty && ctx.store && ctx.defaults) ctx.store->save(ctx.cfg->device, *ctx.defaults);
  _dirty = false;
}

void SettingsScene::resetToDefaults(AppContext& ctx) {
  if (!ctx.defaults) return;
  auto& d = ctx.cfg->device;
  const bool flipChanged = d.flip != ctx.defaults->flip;
  d = *ctx.defaults;
  if (ctx.store) ctx.store->clear();
  if (flipChanged) ui::applyFlip(d.flip);
  ctx.locked = d.eventModeEnabled;
  if (ctx.power) ctx.power->refresh();
  _dirty = false;
}

float SettingsScene::maxScroll() const { return std::max(0.0f, static_cast<float>(_contentH - (kBottom - kTop))); }

int SettingsScene::rowTop(int index) const {
  int y = kTop - static_cast<int>(std::lround(_scroll));
  for (int i = 0; i < index; ++i) y += _rows[i].h;
  return y;
}

int SettingsScene::rowAt(int y) const {
  if (y < kTop || y > kBottom) return -1;
  for (int i = 0; i < static_cast<int>(_rows.size()); ++i) {
    const int t = rowTop(i);
    if (y >= t && y < t + _rows[i].h) return i;
  }
  return -1;
}

bool SettingsScene::inSliderBand(int index, int y) const {
  if (index < 0 || _rows[index].kind != Kind::Slider) return false;
  const int t = rowTop(index);
  return y >= t + 36 && y <= t + _rows[index].h;
}

int SettingsScene::sliderValueAt(const Row& r, int x) const {
  float ratio = static_cast<float>(x - kTrackX0) / (kTrackX1 - kTrackX0);
  ratio = std::min(std::max(ratio, 0.0f), 1.0f);
  const int steps = (r.max - r.min) / r.step;
  const int v = r.min + static_cast<int>(std::lround(ratio * steps)) * r.step;
  return std::min(std::max(v, r.min), r.max);
}

int SettingsScene::value(AppContext& ctx, Field f) const {
  const auto& d = ctx.cfg->device;
  switch (f) {
    case Field::Flip: return d.flip ? 1 : 0;
    case Field::Brightness: return d.brightness;
    case Field::Event: return d.eventModeEnabled ? 1 : 0;
    case Field::EventBrightness: return d.eventBrightness;
    case Field::PixelShift: return d.pixelShift ? 1 : 0;
    case Field::Dim: return static_cast<int>(d.dimAfterSec);
    case Field::Off: return static_cast<int>(d.offAfterSec);
    case Field::Standby: return static_cast<int>(d.standbyAfterMin);
    case Field::Battery: return ui::batteryPercent();
    default: return 0;
  }
}

void SettingsScene::setValue(AppContext& ctx, Field f, int v) {
  auto& d = ctx.cfg->device;
  if (value(ctx, f) == v) return;
  switch (f) {
    case Field::Flip:
      d.flip = (v != 0);
      ui::applyFlip(d.flip);
      break;
    case Field::Brightness: d.brightness = static_cast<uint8_t>(v); break;
    case Field::Event:
      d.eventModeEnabled = (v != 0);
      ctx.locked = d.eventModeEnabled;  // ON にしたら Badge に戻ったときからロック
      break;
    case Field::EventBrightness: d.eventBrightness = static_cast<uint8_t>(v); break;
    case Field::PixelShift: d.pixelShift = (v != 0); break;
    case Field::Dim: d.dimAfterSec = static_cast<uint32_t>(v); break;
    case Field::Off: d.offAfterSec = static_cast<uint32_t>(v); break;
    case Field::Standby: d.standbyAfterMin = static_cast<uint32_t>(v); break;
    default: return;
  }
  _dirty = true;
  if (ctx.power) ctx.power->refresh();  // Menu 中は輝度を変えない(出たときに反映)
}

String SettingsScene::valueText(AppContext& ctx, const Row& r, int v) const {
  switch (r.field) {
    case Field::Dim:
    case Field::Off: return v == 0 ? String("しない") : String(v) + " 秒";
    case Field::Standby: return v == 0 ? String("しない") : String(v) + " 分";
    case Field::Battery: return String(v) + "%" + (ui::batteryCharging() ? " 充電中" : "");
    case Field::Version: return APP_VERSION;
    default: (void)ctx; return String(v);
  }
}

void SettingsScene::render(AppContext& ctx) {
  using namespace ui;
  clear();
  auto& f = frame();
  statusBar();
  text("Settings", kCx, 64, 20, kTextSub);

  f.setClipRect(0, kTop, kW, kBottom - kTop);
  for (int i = 0; i < static_cast<int>(_rows.size()); ++i) {
    const Row& r = _rows[i];
    const int y0 = rowTop(i);
    if (y0 + r.h < kTop || y0 > kBottom) continue;
    const int v = value(ctx, r.field);
    switch (r.kind) {
      case Kind::Segment: {
        const int cy = y0 + r.h / 2;
        text(r.label, kLeft, cy, 20, kText, middle_left);
        const int sx = kRight - kSegW;
        const int sy = cy - kSegH / 2;
        f.fillRoundRect(sx, sy, kSegW, kSegH, kSegH / 2, kTrack);
        const bool left = (v != 0);  // 左 = 首掛け(反転)、右 = 手持ち
        f.fillRoundRect(left ? sx : sx + kSegW / 2, sy, kSegW / 2, kSegH, kSegH / 2, kAccentA);
        text("首掛け", sx + kSegW / 4, cy, 16, left ? kBlack : kTextSub);
        text("手持ち", sx + kSegW * 3 / 4, cy, 16, left ? kTextSub : kBlack);
        break;
      }
      case Kind::Toggle: {
        const int cy = y0 + r.h / 2;
        text(r.label, kLeft, cy, 20, kText, middle_left);
        const int sx = kRight - kSwitchW;
        const int sy = cy - kSwitchH / 2;
        f.fillRoundRect(sx, sy, kSwitchW, kSwitchH, kSwitchH / 2, v ? kGood : kTrack);
        f.fillCircle(v ? sx + kSwitchW - kSwitchH / 2 : sx + kSwitchH / 2, cy, kSwitchH / 2 - 4, v ? kWhite : kTextSub);
        break;
      }
      case Kind::Slider: {
        const bool active = (i == _activeSlider);
        text(r.label, kLeft, y0 + 24, 20, kText, middle_left);
        text(valueText(ctx, r, v), kRight, y0 + 24, 20, active ? kAccentA : kTextSub, middle_right);
        const int ty = y0 + 58;
        float ratio = (r.max > r.min) ? static_cast<float>(v - r.min) / (r.max - r.min) : 0.0f;
        ratio = std::min(std::max(ratio, 0.0f), 1.0f);
        const int kx = kTrackX0 + static_cast<int>(std::lround(ratio * (kTrackX1 - kTrackX0)));
        f.fillRoundRect(kTrackX0, ty - 3, kTrackX1 - kTrackX0, 6, 3, kTrack);
        if (kx > kTrackX0) f.fillRoundRect(kTrackX0, ty - 3, kx - kTrackX0, 6, 3, kAccentA);
        f.fillCircle(kx, ty, active ? 14 : 11, active ? kAccentA : kText);
        break;
      }
      case Kind::Action: {
        const int cy = y0 + r.h / 2;
        const bool armed = _resetArmedAt != 0 && ui::elapsedMs(millis(), _resetArmedAt) < 3000;
        text(r.label, kLeft, cy, 20, armed ? kAccentA : kText, middle_left);
        text(armed ? "もう一度タップ" : "タップ", kRight, cy, 16, armed ? kAccentA : kTextSub, middle_right);
        break;
      }
      case Kind::Info: {
        const int cy = y0 + r.h / 2;
        text(r.label, kLeft, cy, 20, kText, middle_left);
        text(valueText(ctx, r, v), kRight, cy, 20, kTextSub, middle_right);
        break;
      }
    }
    if (i + 1 < static_cast<int>(_rows.size())) f.drawFastHLine(kLeft, y0 + r.h - 1, kRight - kLeft, kTrack);
  }
  f.clearClipRect();

  // 右の縁にスクロール位置(3 時の前後 30°)
  const float ms = maxScroll();
  if (ms > 0) {
    const float span = 30.0f * (kBottom - kTop) / _contentH;
    const float start = 75.0f + (30.0f - span) * (_scroll / ms);
    arc(208, 213, 75, 105, kTrack);
    arc(208, 213, start, start + span, kTextSub);
  }

  buttonHints(nullptr, "戻る");
  present();
  _lastStatus = millis();
}

SceneId SettingsScene::tick(AppContext& ctx, uint32_t now) {
  if (_resetArmedAt != 0 && ui::elapsedMs(now, _resetArmedAt) >= 3000) {
    _resetArmedAt = 0;
    render(ctx);
  }
  if (_activeSlider < 0 && !_scrolling && ui::elapsedMs(now, _lastStatus) >= 10000) render(ctx);
  return SceneId::Same;
}

SceneId SettingsScene::onInput(AppContext& ctx, const InputEvent& ev) {
  switch (ev.type) {
    case InputEvent::KeyBClick:
      return SceneId::Launcher;
    case InputEvent::DragStart: {
      const int i = rowAt(ev.y);
      const bool horizontal = std::abs(ev.dx) >= 2 * std::abs(ev.dy);
      if (horizontal && inSliderBand(i, ev.y)) {
        _activeSlider = i;
        setValue(ctx, _rows[i].field, sliderValueAt(_rows[i], ev.x));
        render(ctx);
      } else if (!horizontal && ev.y >= kTop && ev.y <= kBottom) {
        _scrolling = true;
      }
      break;
    }
    case InputEvent::DragMove:
      if (_activeSlider >= 0) {
        setValue(ctx, _rows[_activeSlider].field, sliderValueAt(_rows[_activeSlider], ev.x));
        render(ctx);
      } else if (_scrolling) {
        _scroll = std::min(std::max(_scroll - ev.dy, 0.0f), maxScroll());
        render(ctx);
      }
      break;
    case InputEvent::DragEnd:
      if (_activeSlider >= 0 || _scrolling) {
        if (_activeSlider >= 0) log_i("[settings] %s = %d", _rows[_activeSlider].label, value(ctx, _rows[_activeSlider].field));
        commit(ctx);
        _activeSlider = -1;
        _scrolling = false;
        render(ctx);
      }
      break;
    case InputEvent::Tap: {
      const int i = rowAt(ev.y);
      if (i < 0 || ev.x < kLeft - 24 || ev.x > kRight + 24) break;
      const Row& r = _rows[i];
      switch (r.kind) {
        case Kind::Toggle:
          setValue(ctx, r.field, value(ctx, r.field) ? 0 : 1);
          break;
        case Kind::Segment:
          setValue(ctx, r.field, ev.x < kRight - kSegW / 2 ? 1 : 0);
          break;
        case Kind::Slider:
          if (inSliderBand(i, ev.y)) setValue(ctx, r.field, sliderValueAt(r, ev.x));
          break;
        case Kind::Action:
          if (_resetArmedAt != 0 && ui::elapsedMs(millis(), _resetArmedAt) < 3000) {
            _resetArmedAt = 0;
            resetToDefaults(ctx);
          } else {
            _resetArmedAt = millis();
          }
          break;
        default:
          break;
      }
      log_i("[settings] %s = %d", r.label, value(ctx, r.field));
      commit(ctx);
      render(ctx);
      break;
    }
    default:
      break;
  }
  return SceneId::Same;
}
