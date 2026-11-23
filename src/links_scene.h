// リンク選択: そのプロファイルの SNS を縦の一覧で出す。中央の行が選択中。
// 行のタップ / KEYA = その QR、縦ドラッグ = スクロール、KEYB = Badge へ。
#pragma once
#include "list_wheel.h"
#include "scene.h"

class LinksScene : public Scene {
 public:
  void enter(AppContext& ctx) override;
  SceneId tick(AppContext& ctx, uint32_t now) override;
  SceneId onInput(AppContext& ctx, const InputEvent& ev) override;

 private:
  void render(AppContext& ctx);
  ListWheel _wheel;
  uint32_t _lastStatus = 0;
};
