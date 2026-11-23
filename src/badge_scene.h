// Badge 画面: アバターを画面のちょうど中心に。外輪、12 時付近のプロファイル位置、下に名前。
// タップ / KEYA = リンク一覧、KEYB = ランチャー、左右スワイプ = プロファイル切替、画面長押し = イベントモードの切替。
// イベントモード(常時表示)中はロックし、A+B 同時押しで解除。無操作 15 秒でまたロック。
#pragma once
#include "scene.h"

class BadgeScene : public Scene {
 public:
  void enter(AppContext& ctx) override;
  SceneId tick(AppContext& ctx, uint32_t now) override;
  SceneId onInput(AppContext& ctx, const InputEvent& ev) override;

 private:
  void render(AppContext& ctx, uint32_t now);
  void toast(const char* s, uint32_t ms);

  int _dx = 0;  // ピクセルシフト
  int _dy = 0;
  uint32_t _lastShift = 0;
  uint32_t _batteryUntil = 0;  // この時刻まで外輪を電池残量にする
  bool _dialog = false;        // イベントモードの切替ダイアログ
  String _toast;               // 上部に短く出す文言
  uint32_t _toastUntil = 0;
};
