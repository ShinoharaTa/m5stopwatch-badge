// ボタンとタッチから画面向けのイベントを作る(タップ、長押し、ドラッグ、スワイプ、A+B 同時押し)。
#pragma once
#include <Arduino.h>

#include "scene.h"

namespace input {

constexpr int kMaxEvents = 4;

struct Frame {
  InputEvent events[kMaxEvents];
  int count = 0;
  bool pressStarted = false;  // このフレームでボタンかタッチが押され始めた
  bool anyPressed = false;    // どれかが押されている
  void push(const InputEvent& ev) {
    if (count < kMaxEvents) events[count++] = ev;
  }
};

void begin();
void cancelComboHome();  // 今の A+B 押下では GoHome を出さない(ロック解除に使った押下)
Frame poll(uint32_t now);  // M5.update() のあとに毎フレーム呼ぶ

}  // namespace input
