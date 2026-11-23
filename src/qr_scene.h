// QR 全画面(案 B): 白い円の上に QR(ECC M)。上端に位置の点と種別のピル、下に URL。
// KEYA = 次のリンク、KEYB = リンク一覧へ、スワイプ = 前後、60 秒放置で Badge へ。
#pragma once
#include "scene.h"

class QrScene : public Scene {
 public:
  void enter(AppContext& ctx) override;
  SceneId tick(AppContext& ctx, uint32_t now) override;
  SceneId onInput(AppContext& ctx, const InputEvent& ev) override;

 private:
  void render(AppContext& ctx);
  uint32_t _shownAt = 0;
};
