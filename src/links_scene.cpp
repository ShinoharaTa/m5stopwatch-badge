#include "links_scene.h"

#include "ui.h"

namespace {
constexpr int kRowH = 64;
constexpr int kCenterY = 240;
constexpr int kTop = 92;
constexpr int kBottom = 392;
constexpr int kRowX = 84;
constexpr int kRowW = 300;

String defaultLabel(const String& url) {
  String s = url;
  if (s.startsWith("https://")) s = s.substring(8);
  else if (s.startsWith("http://")) s = s.substring(7);
  if (s.startsWith("www.")) s = s.substring(4);
  while (s.endsWith("/")) s.remove(s.length() - 1);
  return s;
}

String kindName(const LinkConfig& l) {
  const ui::KindStyle st = ui::kindStyle(l.kind);
  if (st.name && st.name[0]) return st.name;
  return l.label.length() ? l.label : l.kind;
}
}  // namespace

void LinksScene::enter(AppContext& ctx) {
  _wheel.setup(static_cast<int>(ctx.profile().links.size()), kRowH, kCenterY, kTop, kBottom);
  _wheel.setFocus(ctx.linkIndex);
  render(ctx);
}

void LinksScene::render(AppContext& ctx) {
  using namespace ui;
  const auto& p = ctx.profile();
  clear();
  auto& f = frame();
  statusBar();
  text(fit(p.name, chordWidth(66) - 40, 20), kCx, 66, 20, kTextSub);

  f.setClipRect(0, kTop, kW, kBottom - kTop);
  _wheel.draw([&](int i, int y, float dist) {
    const auto& l = p.links[i];
    const bool focused = dist < 0.5f;
    if (focused) {
      f.fillRoundRect(kRowX, y - 29, kRowW, 58, 29, kPanel);
      f.drawRoundRect(kRowX, y - 29, kRowW, 58, 29, scale(kAccentA, 0.8f));
      kindIcon(l.kind, l.label, kRowX + 34, y, 21, true);
      text(kindName(l), kRowX + 66, y - 10, 24, kText, middle_left);
      const String handle = l.label.length() ? l.label : defaultLabel(l.url);
      text(fit(handle, kRowW - 66 - 24, 16), kRowX + 66, y + 14, 16, kTextSub, middle_left);
    } else {
      const Color c = dist < 1.5f ? kTextSub : scale(kTextSub, 0.55f);
      kindIcon(l.kind, l.label, kRowX + 34, y, 17, false);
      text(kindName(l), kRowX + 66, y, 20, c, middle_left);
    }
  });
  f.clearClipRect();

  buttonHints("QR", "戻る");
  present();
  _lastStatus = millis();
}

SceneId LinksScene::tick(AppContext& ctx, uint32_t now) {
  if (_wheel.tick(now) || ui::elapsedMs(now, _lastStatus) >= 10000) render(ctx);
  return SceneId::Same;
}

SceneId LinksScene::onInput(AppContext& ctx, const InputEvent& ev) {
  switch (ev.type) {
    case InputEvent::KeyAClick:
      if (_wheel.count() == 0) return SceneId::Same;
      ctx.linkIndex = _wheel.focus();
      return SceneId::Qr;
    case InputEvent::KeyBClick:
      return SceneId::Badge;
    default: {
      int activated = -1;
      if (_wheel.onInput(ev, activated)) {
        ctx.linkIndex = _wheel.focus();
        if (activated >= 0) {
          ctx.linkIndex = activated;
          return SceneId::Qr;
        }
        render(ctx);
      }
      return SceneId::Same;
    }
  }
}
