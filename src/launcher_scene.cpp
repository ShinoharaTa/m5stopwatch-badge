#include "launcher_scene.h"

namespace {
constexpr int kRowH = 64;
constexpr int kCenterY = 240;
constexpr int kTop = 92;
constexpr int kBottom = 392;
constexpr int kRowX = 84;
constexpr int kRowW = 300;
}  // namespace

LauncherScene::LauncherScene() {
  _items = {
      {ui::AppIcon::Badge, SceneId::Badge},
      {ui::AppIcon::Clock, SceneId::Clock},
      {ui::AppIcon::Stopwatch, SceneId::Stopwatch},
      {ui::AppIcon::Settings, SceneId::Settings},
  };
}

void LauncherScene::enter(AppContext& ctx) {
  const int keep = _wheel.focus();
  _wheel.setup(static_cast<int>(_items.size()), kRowH, kCenterY, kTop, kBottom);
  _wheel.setFocus(keep);
  render(ctx);
}

void LauncherScene::render(AppContext& ctx) {
  using namespace ui;
  (void)ctx;
  clear();
  auto& f = frame();
  statusBar();
  text("アプリ", kCx, 66, 20, kTextSub);

  f.setClipRect(0, kTop, kW, kBottom - kTop);
  _wheel.draw([&](int i, int y, float dist) {
    const Item& it = _items[i];
    const Color c = appColor(it.icon);
    const bool focused = dist < 0.5f;
    const int cx = kRowX + 34;
    if (focused) {
      f.fillRoundRect(kRowX, y - 29, kRowW, 58, 29, kPanel);
      f.drawRoundRect(kRowX, y - 29, kRowW, 58, 29, scale(kAccentA, 0.8f));
      f.fillCircle(cx, y, 21, scale(c, 0.32f));
      appIcon(it.icon, cx, y, 14, c, scale(c, 0.32f));
      text(appName(it.icon), kRowX + 66, y, 24, kText, middle_left);
    } else {
      const Color tc = dist < 1.5f ? kTextSub : scale(kTextSub, 0.55f);
      f.fillCircle(cx, y, 17, scale(c, 0.22f));
      appIcon(it.icon, cx, y, 11, scale(c, 0.8f), scale(c, 0.22f));
      text(appName(it.icon), kRowX + 66, y, 20, tc, middle_left);
    }
  });
  f.clearClipRect();

  buttonHints("起動", "戻る");
  present();
  _lastStatus = millis();
}

SceneId LauncherScene::tick(AppContext& ctx, uint32_t now) {
  if (_wheel.tick(now) || ui::elapsedMs(now, _lastStatus) >= 10000) render(ctx);
  return SceneId::Same;
}

SceneId LauncherScene::onInput(AppContext& ctx, const InputEvent& ev) {
  switch (ev.type) {
    case InputEvent::KeyAClick:
      return _items[_wheel.focus()].target;
    case InputEvent::KeyBClick:
      return SceneId::Badge;
    default: {
      int activated = -1;
      if (_wheel.onInput(ev, activated)) {
        if (activated >= 0) return _items[activated].target;
        render(ctx);
      }
      return SceneId::Same;
    }
  }
}
