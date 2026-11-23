// まだ無いアプリの置き場(Clock / Stopwatch)。中央にアイコンと名前、KEYB で戻る。
#pragma once
#include "scene.h"
#include "ui.h"

class PlaceholderScene : public Scene {
 public:
  explicit PlaceholderScene(ui::AppIcon icon) : _icon(icon) {}
  void enter(AppContext& ctx) override;
  SceneId tick(AppContext& ctx, uint32_t now) override;
  SceneId onInput(AppContext& ctx, const InputEvent& ev) override;

 private:
  ui::AppIcon _icon;
  uint32_t _lastStatus = 0;
};
