// Settings: 縦にスクロールする一覧。各行のトグル・切替・シークバーをその場で操作する(編集モードなし)。
// 縦ドラッグ = スクロール、シークバーは横ドラッグかタップ、トグルは行のタップ、KEYB = 戻る。変更は操作のたびに保存。
// 操作中は輝度を一定に保つ(輝度の変更は Settings を出たときに反映)。
#pragma once
#include <vector>

#include "scene.h"

class SettingsScene : public Scene {
 public:
  void enter(AppContext& ctx) override;
  void leave(AppContext& ctx) override;
  SceneId tick(AppContext& ctx, uint32_t now) override;
  SceneId onInput(AppContext& ctx, const InputEvent& ev) override;

 private:
  enum class Kind { Segment, Slider, Toggle, Action, Info };
  enum class Field { Flip, Brightness, Event, EventBrightness, PixelShift, Dim, Off, Standby, Reset, Battery, Version };
  struct Row {
    Field field;
    Kind kind;
    const char* label;
    int min;
    int max;
    int step;
    int h;
  };

  int value(AppContext& ctx, Field f) const;
  void setValue(AppContext& ctx, Field f, int v);
  String valueText(AppContext& ctx, const Row& r, int v) const;
  int rowTop(int index) const;
  int rowAt(int y) const;
  int sliderValueAt(const Row& r, int x) const;
  bool inSliderBand(int index, int y) const;
  float maxScroll() const;
  void render(AppContext& ctx);
  void commit(AppContext& ctx);  // 変更があれば NVS に保存(スタンバイで消えないよう、操作のたびに呼ぶ)
  void resetToDefaults(AppContext& ctx);

  std::vector<Row> _rows;
  int _contentH = 0;
  float _scroll = 0;
  int _activeSlider = -1;
  bool _scrolling = false;
  bool _dirty = false;
  uint32_t _lastStatus = 0;
  uint32_t _resetArmedAt = 0;  // 「初期値に戻す」を 1 回押した時刻(3 秒以内にもう 1 回で実行)
};
