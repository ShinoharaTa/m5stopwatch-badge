#include "placeholder_scene.h"

void PlaceholderScene::enter(AppContext& ctx) {
  using namespace ui;
  (void)ctx;
  clear();
  statusBar();
  const Color c = appColor(_icon);
  frame().fillCircle(kCx, 206, 62, scale(c, 0.32f));
  appIcon(_icon, kCx, 206, 40, c, scale(c, 0.32f));
  text(appName(_icon), kCx, 300, 28, kText);
  text("第 2 段階で実装", kCx, 334, 20, kTextSub);
  buttonHints(nullptr, "戻る");
  present();
  _lastStatus = millis();
}

SceneId PlaceholderScene::tick(AppContext& ctx, uint32_t now) {
  if (ui::elapsedMs(now, _lastStatus) >= 10000) enter(ctx);
  return SceneId::Same;
}

SceneId PlaceholderScene::onInput(AppContext& ctx, const InputEvent& ev) {
  (void)ctx;
  if (ev.type == InputEvent::KeyBClick) return SceneId::Launcher;
  return SceneId::Same;
}
