// 縦に並ぶ一覧(ホイール)。選択中の行が画面の中央に来る。縦ドラッグでスクロール、行のタップで起動、
// 指を離すと一番近い行に吸い付く。リンク選択とランチャーで使う。
#pragma once
#include <Arduino.h>

#include <cmath>

#include "scene.h"

class ListWheel {
 public:
  void setup(int count, int rowH, int centerY, int top, int bottom);
  void setFocus(int index);  // 吸い付きのアニメーションなしで合わせる
  int focus() const;
  int count() const { return _n; }

  // drawRow(index, 行の中心 y, 中央からの距離[行])。呼ぶ前に [top, bottom] でクリップしておく
  template <typename F>
  void draw(F&& drawRow) const {
    for (int i = 0; i < _n; ++i) {
      const float y = _centerY + i * _rowH - _offset;
      if (y < _top - _rowH || y > _bottom + _rowH) continue;
      drawRow(i, static_cast<int>(std::lround(y)), std::fabs(y - _centerY) / _rowH);
    }
  }

  // 描き直しが要れば true。タップで起動する行があれば activated に入れる(なければ -1)
  bool onInput(const InputEvent& ev, int& activated);
  bool tick(uint32_t now);  // 吸い付きのアニメーション中は true

 private:
  float maxOffset() const { return _n > 1 ? static_cast<float>((_n - 1) * _rowH) : 0.0f; }
  int indexAt(int y) const;
  void snapTo(float target);

  int _n = 0;
  int _rowH = 64;
  int _centerY = 240;
  int _top = 92;
  int _bottom = 392;
  float _offset = 0;
  bool _dragging = false;
  bool _anim = false;
  float _animFrom = 0;
  float _animTo = 0;
  uint32_t _animStart = 0;
};
