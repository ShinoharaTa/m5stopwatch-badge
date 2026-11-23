#include "qr_scene.h"

#include <vector>

#include "ui.h"

extern "C" {
#include <lgfx/utility/lgfx_qrcode.h>
}

namespace {
constexpr int kQuietModules = 4;
constexpr int kMaxModulePx = 12;
constexpr int kInnerSquare = 329;  // 233·√2: 円に内接する正方形の一辺

String defaultLabel(const String& url) {
  String s = url;
  if (s.startsWith("https://")) s = s.substring(8);
  else if (s.startsWith("http://")) s = s.substring(7);
  if (s.startsWith("www.")) s = s.substring(4);
  while (s.endsWith("/")) s.remove(s.length() - 1);
  return s;
}

// (N + 8)·m ≤ 329 を満たす最大の整数 m で中心に描く
bool drawQr(const char* textData, int& outVersion, int& outPx) {
  auto& f = ui::frame();
  for (uint8_t v = 1; v <= 20; ++v) {
    QRCode qr;
    std::vector<uint8_t> buf(lgfx_qrcode_getBufferSize(v));
    if (lgfx_qrcode_initText(&qr, buf.data(), v, ECC_MEDIUM, textData) != 0) continue;
    const int n = qr.size;
    int px = kInnerSquare / (n + 2 * kQuietModules);
    if (px < 2) return false;
    if (px > kMaxModulePx) px = kMaxModulePx;
    const int side = n * px;
    const int x0 = ui::kCx - side / 2;
    const int y0 = ui::kCy - side / 2;
    for (int y = 0; y < n; ++y) {
      for (int x = 0; x < n; ++x) {
        if (lgfx_qrcode_getModule(&qr, x, y)) f.fillRect(x0 + x * px, y0 + y * px, px, px, ui::kBlack);
      }
    }
    outVersion = v;
    outPx = px;
    return true;
  }
  return false;
}
}  // namespace

void QrScene::enter(AppContext& ctx) { render(ctx); }

SceneId QrScene::tick(AppContext& ctx, uint32_t now) {
  const uint32_t timeoutMs = ctx.cfg->device.qrTimeoutSec * 1000UL;
  if (timeoutMs > 0 && ui::elapsedMs(now, _shownAt) >= timeoutMs) return SceneId::Badge;
  return SceneId::Same;
}

SceneId QrScene::onInput(AppContext& ctx, const InputEvent& ev) {
  const int n = static_cast<int>(ctx.profile().links.size());
  if (n == 0) return SceneId::Badge;
  switch (ev.type) {
    case InputEvent::KeyAClick:
    case InputEvent::SwipeLeft:
      ctx.linkIndex = (ctx.linkIndex + 1) % n;
      render(ctx);
      break;
    case InputEvent::SwipeRight:
      ctx.linkIndex = (ctx.linkIndex + n - 1) % n;
      render(ctx);
      break;
    case InputEvent::KeyBClick:
      return SceneId::Links;
    default:
      break;
  }
  return SceneId::Same;
}

void QrScene::render(AppContext& ctx) {
  using namespace ui;
  _shownAt = millis();
  const auto& links = ctx.profile().links;
  const auto& link = links[ctx.linkIndex];
  const int n = static_cast<int>(links.size());

  clear();
  auto& f = frame();
  f.fillCircle(kCx, kCy, kR, kWhite);  // 白べた塗りは QR のときだけ

  // 上端の位置の点(半径 214、6° 間隔)
  for (int i = 0; i < n; ++i) {
    const float a = (i - (n - 1) / 2.0f) * 6.0f;
    int x, y;
    polar(214, a, x, y);
    if (i == ctx.linkIndex) f.fillCircle(x, y, 5, rgb(40, 40, 40));
    else f.fillCircle(x, y, 3, rgb(190, 190, 190));
  }

  // 種別のピル
  const KindStyle st = kindStyle(link.kind);
  const String name = (st.name && st.name[0]) ? String(st.name) : link.kind;
  const int tw = textWidth(name, 20);
  f.fillRoundRect(kCx - (tw + 36) / 2, 62 - 16, tw + 36, 32, 16, st.color);
  text(name, kCx, 62, 20, st.textColor);

  int version = 0, px = 0;
  if (!drawQr(link.url.c_str(), version, px)) {
    text("URL が長すぎます", kCx, kCy, 24, kBlack);
  } else {
    log_i("[qr] %s v%d px=%d len=%u", link.kind.c_str(), version, px, static_cast<unsigned>(link.url.length()));
  }

  const String label = link.label.length() ? link.label : defaultLabel(link.url);
  text(fit(label, chordWidth(404) - 32, 16), kCx, 404, 16, rgb(60, 60, 60));
  present();
}
