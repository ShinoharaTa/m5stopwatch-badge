// ランチャー: アプリを縦の一覧で出す。中央の行が選択中。
// 行のタップ / KEYA = 起動、縦ドラッグ = スクロール、KEYB = Badge へ。
#pragma once
#include <vector>

#include "list_wheel.h"
#include "scene.h"
#include "ui.h"

class LauncherScene : public Scene {
 public:
  LauncherScene();
  void enter(AppContext& ctx) override;
  SceneId tick(AppContext& ctx, uint32_t now) override;
  SceneId onInput(AppContext& ctx, const InputEvent& ev) override;

 private:
  struct Item {
    ui::AppIcon icon;
    SceneId target;
  };
  void render(AppContext& ctx);

  std::vector<Item> _items;
  ListWheel _wheel;
  uint32_t _lastStatus = 0;
};
