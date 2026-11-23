#include "input.h"

#include <M5Unified.h>

#include <cstdlib>

#include "ui.h"

namespace input {
namespace {
constexpr uint32_t kHoldMs = 800;       // ボタン長押しと A+B 長押し
constexpr uint32_t kTouchHoldMs = 700;  // 画面の長押し
constexpr int kMoveThresh = 18;         // これ以上動いたらドラッグ(約 1.7mm。指のぶれでタップを取りこぼさない)
constexpr int kSwipeThresh = 60;        // 横ドラッグをスワイプとみなす距離

struct Combo {
  bool seen = false;  // 今回の押下で一度でも両方押されたか
  uint32_t since = 0;
  bool homeFired = false;
} g_combo;

struct Touch {
  bool down = false;
  int sx = 0, sy = 0;  // 押し始め
  int lx = 0, ly = 0;  // 前フレーム
  uint32_t t0 = 0;
  bool moved = false;
  bool vertical = false;
  bool holdFired = false;
} g_touch;

InputEvent make(InputEvent::Type t, int x = 0, int y = 0) {
  InputEvent ev;
  ev.type = t;
  ev.x = x;
  ev.y = y;
  return ev;
}

// A+B の同時押し。true を返したら単独のクリック/長押しは捨てる
bool pollCombo(uint32_t now, Frame& f) {
  const bool a = M5.BtnA.isPressed();
  const bool b = M5.BtnB.isPressed();
  if (a && b) {
    if (!g_combo.seen) {
      g_combo.seen = true;
      g_combo.since = now;
      g_combo.homeFired = false;
      f.push(make(InputEvent::BothPress));
    } else if (!g_combo.homeFired && ui::elapsedMs(now, g_combo.since) >= kHoldMs) {
      g_combo.homeFired = true;
      f.push(make(InputEvent::GoHome));
    }
    return true;
  }
  if (!a && !b) {
    const bool wasCombo = g_combo.seen;
    g_combo.seen = false;
    return wasCombo;  // 両方離した瞬間のクリックも捨てる
  }
  return g_combo.seen;  // 片方だけ残っている
}

void pollButtons(uint32_t now, Frame& f) {
  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) f.pressStarted = true;
  if (pollCombo(now, f)) return;
  if (M5.BtnA.wasClicked()) f.push(make(InputEvent::KeyAClick));
  else if (M5.BtnB.wasClicked()) f.push(make(InputEvent::KeyBClick));
  else if (M5.BtnA.wasHold()) f.push(make(InputEvent::KeyAHold));
  else if (M5.BtnB.wasHold()) f.push(make(InputEvent::KeyBHold));
}

void pollTouch(uint32_t now, Frame& f) {
  if (!M5.Touch.isEnabled()) return;
  const auto& t = M5.Touch.getDetail();
  const bool pressed = t.isPressed();

  if (pressed && !g_touch.down) {
    g_touch = Touch{};
    g_touch.down = true;
    g_touch.sx = g_touch.lx = t.x;
    g_touch.sy = g_touch.ly = t.y;
    g_touch.t0 = now;
    f.pressStarted = true;
    return;
  }

  if (pressed && g_touch.down) {
    const int tdx = t.x - g_touch.sx;
    const int tdy = t.y - g_touch.sy;
    if (!g_touch.moved && !g_touch.holdFired && (std::abs(tdx) >= kMoveThresh || std::abs(tdy) >= kMoveThresh)) {
      g_touch.moved = true;
      g_touch.vertical = std::abs(tdy) > std::abs(tdx);
      InputEvent ev = make(InputEvent::DragStart, g_touch.sx, g_touch.sy);
      ev.dx = tdx;  // 判定した時点の移動量(向きの強さを画面側で見られるように)
      ev.dy = tdy;
      ev.vertical = g_touch.vertical;
      f.push(ev);
    }
    if (g_touch.moved) {
      const int dx = t.x - g_touch.lx;
      const int dy = t.y - g_touch.ly;
      if (dx != 0 || dy != 0) {
        InputEvent ev = make(InputEvent::DragMove, t.x, t.y);
        ev.dx = dx;
        ev.dy = dy;
        ev.vertical = g_touch.vertical;
        f.push(ev);
      }
    } else if (!g_touch.holdFired && ui::elapsedMs(now, g_touch.t0) >= kTouchHoldMs) {
      g_touch.holdFired = true;
      f.push(make(InputEvent::TouchHold, g_touch.sx, g_touch.sy));
    }
    g_touch.lx = t.x;
    g_touch.ly = t.y;
    return;
  }

  if (!pressed && g_touch.down) {
    g_touch.down = false;
    if (g_touch.moved) {
      InputEvent ev = make(InputEvent::DragEnd, g_touch.lx, g_touch.ly);
      ev.dx = g_touch.lx - g_touch.sx;
      ev.dy = g_touch.ly - g_touch.sy;
      ev.vertical = g_touch.vertical;
      f.push(ev);
      if (!g_touch.vertical && std::abs(ev.dx) >= kSwipeThresh) {
        f.push(make(ev.dx < 0 ? InputEvent::SwipeLeft : InputEvent::SwipeRight, g_touch.lx, g_touch.ly));
      }
    } else if (!g_touch.holdFired) {
      f.push(make(InputEvent::Tap, g_touch.sx, g_touch.sy));
    }
  }
}
}  // namespace

void cancelComboHome() {
  if (g_combo.seen) g_combo.homeFired = true;
}

void begin() {
  M5.BtnA.setHoldThresh(kHoldMs);
  M5.BtnB.setHoldThresh(kHoldMs);
}

Frame poll(uint32_t now) {
  Frame f;
  pollButtons(now, f);
  pollTouch(now, f);
  f.anyPressed = M5.BtnA.isPressed() || M5.BtnB.isPressed() ||
                 (M5.Touch.isEnabled() && M5.Touch.getDetail().isPressed());
  return f;
}

}  // namespace input
