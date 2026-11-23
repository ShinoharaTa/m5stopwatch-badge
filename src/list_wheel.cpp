#include "list_wheel.h"

#include <algorithm>

#include "ui.h"

namespace {
constexpr uint32_t kSnapMs = 150;
constexpr int kRowX0 = 70;   // タップを受ける横の範囲
constexpr int kRowX1 = 398;
}  // namespace

void ListWheel::setup(int count, int rowH, int centerY, int top, int bottom) {
  _n = count;
  _rowH = rowH;
  _centerY = centerY;
  _top = top;
  _bottom = bottom;
  _dragging = false;
  _anim = false;
  _offset = std::min(std::max(_offset, 0.0f), maxOffset());
}

void ListWheel::setFocus(int index) {
  if (_n <= 0) return;
  index = std::min(std::max(index, 0), _n - 1);
  _offset = static_cast<float>(index * _rowH);
  _anim = false;
}

int ListWheel::focus() const {
  if (_n <= 0) return 0;
  const float off = _anim ? _animTo : _offset;
  const int i = static_cast<int>(std::lround(off / _rowH));
  return std::min(std::max(i, 0), _n - 1);
}

int ListWheel::indexAt(int y) const {
  if (y < _top || y > _bottom || _n <= 0) return -1;
  const int i = static_cast<int>(std::lround((y - _centerY + _offset) / _rowH));
  return (i >= 0 && i < _n) ? i : -1;
}

void ListWheel::snapTo(float target) {
  _animFrom = _offset;
  _animTo = target;
  _animStart = millis();
  _anim = true;
}

bool ListWheel::onInput(const InputEvent& ev, int& activated) {
  activated = -1;
  switch (ev.type) {
    case InputEvent::DragStart:
      if (ev.vertical && ev.y >= _top && ev.y <= _bottom) {
        _dragging = true;
        _anim = false;
      }
      return false;
    case InputEvent::DragMove: {
      if (!_dragging) return false;
      const float overscroll = _rowH * 0.4f;
      _offset = std::min(std::max(_offset - ev.dy, -overscroll), maxOffset() + overscroll);
      return true;
    }
    case InputEvent::DragEnd: {
      if (!_dragging) return false;
      _dragging = false;
      const float clamped = std::min(std::max(_offset, 0.0f), maxOffset());
      snapTo(std::lround(clamped / _rowH) * static_cast<float>(_rowH));
      return true;
    }
    case InputEvent::Tap: {
      if (ev.x < kRowX0 || ev.x > kRowX1) return false;
      const int i = indexAt(ev.y);
      if (i < 0) return false;
      snapTo(static_cast<float>(i * _rowH));
      activated = i;
      return true;
    }
    default:
      return false;
  }
}

bool ListWheel::tick(uint32_t now) {
  if (!_anim) return false;
  const uint32_t t = ui::elapsedMs(now, _animStart);
  if (t >= kSnapMs) {
    _offset = _animTo;
    _anim = false;
    return true;
  }
  const float p = static_cast<float>(t) / kSnapMs;
  const float eased = 1.0f - (1.0f - p) * (1.0f - p);
  _offset = _animFrom + (_animTo - _animFrom) * eased;
  return true;
}
