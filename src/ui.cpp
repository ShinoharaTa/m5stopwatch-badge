#include "ui.h"

#include <cmath>

namespace ui {

Color rgb(uint8_t r, uint8_t g, uint8_t b) { return lgfx::color565(r, g, b); }

Color scale(Color c, float f) {
  const int r = ((c >> 11) & 0x1F) * 255 / 31;
  const int g = ((c >> 5) & 0x3F) * 255 / 63;
  const int b = (c & 0x1F) * 255 / 31;
  auto s = [f](int v) { int o = static_cast<int>(v * f + 0.5f); return static_cast<uint8_t>(o < 0 ? 0 : o > 255 ? 255 : o); };
  return rgb(s(r), s(g), s(b));
}

const Color kBlack = rgb(0, 0, 0);
const Color kWhite = rgb(255, 255, 255);
const Color kText = rgb(240, 240, 240);
const Color kTextSub = rgb(140, 140, 150);
const Color kTextDisabled = rgb(60, 60, 68);
const Color kTrack = rgb(38, 38, 44);
const Color kPanel = rgb(40, 40, 50);
const Color kAccentA = rgb(250, 190, 30);
const Color kAccentB = rgb(60, 130, 255);
const Color kGood = rgb(60, 200, 120);
const Color kBad = rgb(240, 80, 60);

namespace {
M5Canvas g_frame(&M5.Display);
bool g_frameOk = false;
int g_lastDx = 0;
int g_lastDy = 0;
bool g_presented = false;
bool g_flip = false;
constexpr float kDeg2Rad = 3.14159265f / 180.0f;
}  // namespace

bool initFrame() {
  g_frame.setPsram(true);
  g_frame.setColorDepth(16);
  g_frameOk = g_frame.createSprite(kW, kH) != nullptr;
  if (!g_frameOk) log_e("[ui] フレームバッファを確保できない");
  return g_frameOk;
}

M5Canvas& frame() { return g_frame; }

void clear() { g_frame.fillScreen(kBlack); }

void present(int dx, int dy) {
  auto& d = M5.Display;
  d.startWrite();
  g_frame.pushSprite(dx, dy);
  if (g_presented && (dx != g_lastDx || dy != g_lastDy)) {
    if (dy > g_lastDy) d.fillRect(0, g_lastDy, kW, dy - g_lastDy, kBlack);
    if (dy < g_lastDy) d.fillRect(0, dy + kH, kW, g_lastDy - dy, kBlack);
    if (dx > g_lastDx) d.fillRect(g_lastDx, 0, dx - g_lastDx, kH, kBlack);
    if (dx < g_lastDx) d.fillRect(dx + kW, 0, g_lastDx - dx, kH, kBlack);
  }
  d.endWrite();
  g_lastDx = dx;
  g_lastDy = dy;
  g_presented = true;
}

bool flipped() { return g_flip; }

void applyFlip(bool flip) {
  g_flip = flip;
  M5.Display.setRotation(flip ? 2 : 0);
  M5.Display.fillScreen(kBlack);
  g_presented = false;  // 帯消しの基準をリセット
  log_i("[ui] 画面の向き: %s", flip ? "首掛け(反転)" : "手持ち");
}

void polar(float r, float clockDeg, int& x, int& y, int cx, int cy) {
  const float a = (clockDeg - 90.0f) * kDeg2Rad;
  x = cx + static_cast<int>(std::lround(r * std::cos(a)));
  y = cy + static_cast<int>(std::lround(r * std::sin(a)));
}

float clockAngle(int x, int y, int cx, int cy) {
  float deg = std::atan2(static_cast<float>(y - cy), static_cast<float>(x - cx)) / kDeg2Rad + 90.0f;
  while (deg < 0) deg += 360.0f;
  while (deg >= 360.0f) deg -= 360.0f;
  return deg;
}

float wrap180(float deg) {
  while (deg > 180.0f) deg -= 360.0f;
  while (deg <= -180.0f) deg += 360.0f;
  return deg;
}

void arc(float r0, float r1, float fromClockDeg, float toClockDeg, Color c, int cx, int cy) {
  if (toClockDeg <= fromClockDeg) return;
  if (toClockDeg - fromClockDeg >= 360.0f) {
    g_frame.fillArc(cx, cy, static_cast<int>(r0), static_cast<int>(r1), 0.0f, 360.0f, c);
    return;
  }
  // M5GFX は 3 時起点なので -90°。0..360 に正規化し、360 をまたぐなら 2 回に分ける
  float a0 = std::fmod(fromClockDeg - 90.0f, 360.0f);
  if (a0 < 0) a0 += 360.0f;
  float a1 = a0 + (toClockDeg - fromClockDeg);
  if (a1 <= 360.0f) {
    g_frame.fillArc(cx, cy, static_cast<int>(r0), static_cast<int>(r1), a0, a1, c);
  } else {
    g_frame.fillArc(cx, cy, static_cast<int>(r0), static_cast<int>(r1), a0, 360.0f, c);
    g_frame.fillArc(cx, cy, static_cast<int>(r0), static_cast<int>(r1), 0.0f, a1 - 360.0f, c);
  }
}

void ring(float r, float thickness, Color c, int cx, int cy) {
  g_frame.fillArc(cx, cy, static_cast<int>(r - thickness / 2), static_cast<int>(r + thickness / 2), 0.0f, 360.0f, c);
}

int chordWidth(int y, int r) {
  const int dy = y - kCy;
  if (std::abs(dy) >= r) return 0;
  return static_cast<int>(2 * std::sqrt(static_cast<double>(r) * r - static_cast<double>(dy) * dy));
}

const lgfx::IFont* jgFont(int px, float& scaleOut) {
  struct Entry { int px; const lgfx::IFont* font; };
  static const Entry table[] = {
      {8, &fonts::lgfxJapanGothic_8},   {12, &fonts::lgfxJapanGothic_12}, {16, &fonts::lgfxJapanGothic_16},
      {20, &fonts::lgfxJapanGothic_20}, {24, &fonts::lgfxJapanGothic_24}, {28, &fonts::lgfxJapanGothic_28},
      {32, &fonts::lgfxJapanGothic_32}, {36, &fonts::lgfxJapanGothic_36}, {40, &fonts::lgfxJapanGothic_40},
  };
  const Entry* best = &table[0];
  for (const auto& e : table) {
    if (e.px <= px) best = &e;
  }
  scaleOut = static_cast<float>(px) / best->px;
  if (scaleOut < 1.0f) scaleOut = 1.0f;
  return best->font;
}

void text(const char* s, int x, int y, int px, Color c, textdatum_t datum) {
  float sc = 1.0f;
  g_frame.setFont(jgFont(px, sc));
  g_frame.setTextSize(sc);
  g_frame.setTextDatum(datum);
  g_frame.setTextColor(c);
  g_frame.drawString(s, x, y);
  g_frame.setTextSize(1.0f);
}

void text(const String& s, int x, int y, int px, Color c, textdatum_t datum) { text(s.c_str(), x, y, px, c, datum); }

int textWidth(const String& s, int px) {
  float sc = 1.0f;
  g_frame.setFont(jgFont(px, sc));
  g_frame.setTextSize(sc);
  const int w = g_frame.textWidth(s.c_str());
  g_frame.setTextSize(1.0f);
  return w;
}

String fit(const String& s, int maxWidth, int px) {
  if (textWidth(s, px) <= maxWidth) return s;
  String t = s;
  while (t.length() > 2) {
    t.remove(t.length() - 1);
    while (t.length() > 0 && (static_cast<uint8_t>(t[t.length() - 1]) & 0xC0) == 0x80) t.remove(t.length() - 1);
    if (textWidth(t + "...", px) <= maxWidth) return t + "...";
  }
  return t;
}

namespace {
struct Status {
  uint32_t at = 0;
  bool valid = false;
  int pct = 0;
  bool charging = false;
  String time;
} g_status;

void refreshStatus() {
  const uint32_t now = millis();
  if (g_status.valid && elapsedMs(now, g_status.at) < 10000) return;
  const int b = M5.Power.getBatteryLevel();
  g_status.pct = b < 0 ? 0 : b > 100 ? 100 : b;
  g_status.charging = M5.Power.isCharging();
  g_status.time = "";
  if (M5.Rtc.isEnabled()) {
    const auto dt = M5.Rtc.getDateTime();
    if (dt.date.year >= 2024) {
      char buf[8];
      snprintf(buf, sizeof(buf), "%02d:%02d", dt.time.hours, dt.time.minutes);
      g_status.time = buf;
    }
  }
  g_status.at = now;
  g_status.valid = true;
}
}  // namespace

int batteryPercent() {
  refreshStatus();
  return g_status.pct;
}

bool batteryCharging() {
  refreshStatus();
  return g_status.charging;
}

String timeText() {
  refreshStatus();
  return g_status.time;
}

void statusBar(int y) {
  refreshStatus();
  const String pctText = String(g_status.pct) + "%";
  constexpr int iconW = 24, iconH = 12, nub = 3, gap = 6, timeGap = 14;
  const int pctW = textWidth(pctText, 16);
  const int timeW = g_status.time.length() ? textWidth(g_status.time, 16) + timeGap : 0;
  int x = kCx - (timeW + iconW + nub + gap + pctW) / 2;
  if (timeW) {
    text(g_status.time, x, y, 16, kTextSub, middle_left);
    x += timeW;
  }
  const Color fill = g_status.charging ? kAccentA : (g_status.pct < 20 ? kBad : kGood);
  g_frame.drawRoundRect(x, y - iconH / 2, iconW, iconH, 3, kTextSub);
  g_frame.fillRect(x + iconW, y - 3, nub, 6, kTextSub);
  const int fw = (iconW - 4) * g_status.pct / 100;
  if (fw > 0) g_frame.fillRect(x + 2, y - iconH / 2 + 2, fw, iconH - 4, fill);
  x += iconW + nub + gap;
  text(pctText, x, y, 16, kTextSub, middle_left);
}

void buttonHints(const char* keyA, const char* keyB, float dim) {
  // 通常: KEYA は 10〜11 時、KEYB は 1〜2 時。反転(首掛け)では本体ごと回るので KEYA は 4〜5 時、KEYB は 7〜8 時
  const float aArc = g_flip ? 135.0f : 315.0f;
  const float bArc = g_flip ? 225.0f : 45.0f;
  const float aLabel = g_flip ? 116.0f : 296.0f;
  const float bLabel = g_flip ? 244.0f : 64.0f;
  if (keyA) {
    const Color c = scale(kAccentA, dim);
    arc(221, 227, aArc - 9, aArc + 9, c);
    int x, y;
    polar(200, aLabel, x, y);
    text(keyA, x, y, 16, c);
  }
  if (keyB) {
    const Color c = scale(kAccentB, dim);
    arc(221, 227, bArc - 9, bArc + 9, c);
    int x, y;
    polar(200, bLabel, x, y);
    text(keyB, x, y, 16, c);
  }
}

KindStyle kindStyle(const String& kind) {
  if (kind == "x") return {rgb(235, 235, 235), kBlack, "X", "X", false};
  if (kind == "bluesky") return {rgb(17, 133, 254), kWhite, "Bs", "Bluesky", false};
  if (kind == "github") return {rgb(110, 118, 129), kWhite, "GH", "GitHub", false};
  if (kind == "nostr") return {rgb(142, 68, 239), kWhite, "N", "Nostr", false};
  if (kind == "instagram") return {rgb(225, 48, 108), kWhite, "IG", "Instagram", false};
  if (kind == "threads") return {rgb(200, 200, 200), kBlack, "@", "Threads", false};
  if (kind == "mastodon") return {rgb(99, 100, 255), kWhite, "M", "Mastodon", false};
  if (kind == "misskey") return {rgb(134, 179, 0), kWhite, "Mk", "Misskey", false};
  if (kind == "youtube") return {rgb(255, 0, 0), kWhite, "YT", "YouTube", false};
  if (kind == "web") return {rgb(40, 170, 120), kWhite, "", "Web", true};
  return {rgb(90, 90, 100), kWhite, "", "", false};
}

String kindGlyph(const String& kind, const String& label) {
  const KindStyle st = kindStyle(kind);
  if (st.glyph && st.glyph[0]) return st.glyph;
  const String& src = label.length() ? label : kind;
  if (src.length() == 0) return "?";
  const uint8_t c = static_cast<uint8_t>(src[0]);
  const unsigned len = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
  String g = src.substring(0, len);
  g.toUpperCase();
  return g;
}

void kindIcon(const String& kind, const String& label, int cx, int cy, int r, bool selected) {
  const KindStyle st = kindStyle(kind);
  const Color circle = selected ? st.color : scale(st.color, 0.28f);
  const Color fg = selected ? st.textColor : scale(st.color, 0.9f);
  g_frame.fillCircle(cx, cy, r, circle);
  if (st.globe) {
    const int gr = static_cast<int>(r * 0.55f);
    g_frame.drawCircle(cx, cy, gr, fg);
    g_frame.drawEllipse(cx, cy, static_cast<int>(r * 0.25f), gr, fg);
    for (int k = -1; k <= 1; ++k) {
      const int yy = cy + static_cast<int>(k * r * 0.3f);
      const int half = static_cast<int>(std::sqrt(std::max(0.0f, static_cast<float>(gr * gr) - static_cast<float>((yy - cy) * (yy - cy)))));
      g_frame.drawFastHLine(cx - half, yy, half * 2 + 1, fg);
    }
    return;
  }
  const String g = kindGlyph(kind, label);
  const int chars = (g.length() >= 2 && static_cast<uint8_t>(g[0]) < 0x80) ? 2 : 1;
  const int px = static_cast<int>(r * (chars == 1 ? 1.0f : 0.78f));
  text(g, cx, cy + 1, px, fg);
}

Color appColor(AppIcon a) {
  switch (a) {
    case AppIcon::Badge: return rgb(64, 140, 255);
    case AppIcon::Clock: return rgb(255, 170, 60);
    case AppIcon::Stopwatch: return rgb(40, 200, 150);
    case AppIcon::Settings: return rgb(150, 150, 170);
  }
  return kText;
}

const char* appName(AppIcon a) {
  switch (a) {
    case AppIcon::Badge: return "Badge";
    case AppIcon::Clock: return "Clock";
    case AppIcon::Stopwatch: return "Stopwatch";
    case AppIcon::Settings: return "Settings";
  }
  return "";
}

void appIcon(AppIcon a, int cx, int cy, int s, Color fg, Color bg) {
  auto& f = g_frame;
  switch (a) {
    case AppIcon::Badge: {
      f.fillCircle(cx, cy - static_cast<int>(0.25f * s), static_cast<int>(0.26f * s), fg);
      // 胴: 楕円の上半分
      f.fillEllipseArc(cx, cy + static_cast<int>(0.48f * s), 0, static_cast<int>(0.55f * s), 0, static_cast<int>(0.45f * s), 180.0f, 360.0f, fg);
      break;
    }
    case AppIcon::Clock: {
      f.fillArc(cx, cy, static_cast<int>(0.47f * s), static_cast<int>(0.58f * s), 0.0f, 360.0f, fg);
      f.drawWideLine(cx, cy, cx, cy - static_cast<int>(0.34f * s), 0.05f * s, fg);
      f.drawWideLine(cx, cy, cx + static_cast<int>(0.26f * s), cy, 0.05f * s, fg);
      f.fillCircle(cx, cy, static_cast<int>(0.06f * s), fg);
      break;
    }
    case AppIcon::Stopwatch: {
      const int ro = static_cast<int>(0.52f * s);
      f.fillArc(cx, cy + static_cast<int>(0.06f * s), static_cast<int>(0.41f * s), ro, 0.0f, 360.0f, fg);
      f.fillRoundRect(cx - static_cast<int>(0.1f * s), cy + static_cast<int>(0.06f * s) - ro - static_cast<int>(0.16f * s),
                      static_cast<int>(0.2f * s), static_cast<int>(0.16f * s), static_cast<int>(0.04f * s), fg);
      int hx, hy;
      polar(0.3f * s, 40.0f, hx, hy, cx, cy + static_cast<int>(0.06f * s));
      f.drawWideLine(cx, cy + static_cast<int>(0.06f * s), hx, hy, 0.05f * s, fg);
      break;
    }
    case AppIcon::Settings: {
      const float r0 = 0.42f * s;
      const float r1 = 0.6f * s;
      const float half = 0.11f * s;  // 歯の半幅
      for (int i = 0; i < 8; ++i) {
        const float a = i * 45.0f;
        int ax, ay, bx, by, cx2, cy2, dx, dy;
        // 歯の 4 隅: 内側 2 点と外側 2 点(接線方向に ±half)
        const float t = (a - 90.0f) * kDeg2Rad;
        const float ux = std::cos(t), uy = std::sin(t);   // 半径方向
        const float vx = -uy, vy = ux;                    // 接線方向
        ax = cx + static_cast<int>(r0 * ux + half * vx); ay = cy + static_cast<int>(r0 * uy + half * vy);
        bx = cx + static_cast<int>(r0 * ux - half * vx); by = cy + static_cast<int>(r0 * uy - half * vy);
        cx2 = cx + static_cast<int>(r1 * ux - half * 0.7f * vx); cy2 = cy + static_cast<int>(r1 * uy - half * 0.7f * vy);
        dx = cx + static_cast<int>(r1 * ux + half * 0.7f * vx); dy = cy + static_cast<int>(r1 * uy + half * 0.7f * vy);
        f.fillTriangle(ax, ay, bx, by, cx2, cy2, fg);
        f.fillTriangle(ax, ay, cx2, cy2, dx, dy, fg);
      }
      f.fillCircle(cx, cy, static_cast<int>(r0), fg);
      f.fillCircle(cx, cy, static_cast<int>(0.18f * s), bg);
      break;
    }
  }
}

void dialog(const String& body, const char* keyA, const char* keyB) {
  const int w = 320;
  const int h = 150;
  const int x = kCx - w / 2;
  const int y = kCy - h / 2;
  g_frame.fillRoundRect(x, y, w, h, 24, kPanel);
  text(fit(body, w - 32, 20), kCx, y + 48, 20, kText);
  // ピル型のボタン風表示(操作はボタンで)
  const int bw = 120, bh = 40, gap = 20;
  // 物理ボタンと同じ並び(通常は A が左、反転時は A が右)
  const int bx0 = g_flip ? kCx + gap / 2 : kCx - bw - gap / 2;
  const int bx1 = g_flip ? kCx - bw - gap / 2 : kCx + gap / 2;
  const int by = y + h - bh - 22;
  if (keyA) {
    g_frame.fillRoundRect(bx0, by, bw, bh, bh / 2, scale(kAccentA, 0.85f));
    text(String("A ") + keyA, bx0 + bw / 2, by + bh / 2, 20, kBlack);
  }
  if (keyB) {
    g_frame.fillRoundRect(bx1, by, bw, bh, bh / 2, scale(kAccentB, 0.85f));
    text(String("B ") + keyB, bx1 + bw / 2, by + bh / 2, 20, kWhite);
  }
}

}  // namespace ui
