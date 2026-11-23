// 画面共通の部品: フレームバッファ、色、極座標と円弧、文字、ボタン表示、アイコン。
// 座標系は 468×468、中心 (234,234)、見える円の半径 233。角度は時計の角度(0 = 12 時、時計回り)。
#pragma once
// LittleFS.h を M5Unified.h より先に読む(M5GFX のファイル描画対応はこの順でないと有効にならない)
#include <LittleFS.h>
#include <M5Unified.h>

#include <cstdint>

#define APP_VERSION "0.3.0"

namespace ui {

constexpr int kW = 468;
constexpr int kH = 468;
constexpr int kCx = 234;
constexpr int kCy = 234;
constexpr int kR = 233;

using Color = uint16_t;  // RGB565

// 経過ミリ秒。描画中に取った時刻 since がループ先頭の now より後になることがあるので、負なら 0
inline uint32_t elapsedMs(uint32_t now, uint32_t since) {
  return static_cast<int32_t>(now - since) < 0 ? 0 : now - since;
}

Color rgb(uint8_t r, uint8_t g, uint8_t b);
Color scale(Color c, float f);  // 明るさを f 倍(0..1)

extern const Color kBlack;
extern const Color kWhite;
extern const Color kText;          // 主 (240,240,240)
extern const Color kTextSub;       // 副 (140,140,150)
extern const Color kTextDisabled;  // 無効 (60,60,68)
extern const Color kTrack;         // 円弧の下地 (38,38,44)
extern const Color kPanel;         // 選択行・ダイアログ (40,40,50)
extern const Color kAccentA;       // KEYA の黄 (250,190,30)
extern const Color kAccentB;       // KEYB の青 (60,130,255)
extern const Color kGood;          // 電池 (60,200,120)
extern const Color kBad;           // 電池 20% 未満 (240,80,60)

// フレームバッファ(PSRAM)。各画面はここに描き、present() で一括転送する
bool initFrame();
M5Canvas& frame();
void clear();
void present(int dx = 0, int dy = 0);  // ピクセルシフト分ずらして転送し、露出した帯は黒で消す
void applyFlip(bool flip);             // 画面とタッチの向き(true = 180° 回転)
bool flipped();                        // 反転中か(ボタンが画面の下側にある)

// 極座標と円弧
void polar(float r, float clockDeg, int& x, int& y, int cx = kCx, int cy = kCy);
float clockAngle(int x, int y, int cx = kCx, int cy = kCy);  // 点の時計角度 0..360
float wrap180(float deg);                                     // -180..180 に畳む
void arc(float r0, float r1, float fromClockDeg, float toClockDeg, Color c, int cx = kCx, int cy = kCy);
void ring(float r, float thickness, Color c, int cx = kCx, int cy = kCy);
int chordWidth(int y, int r = kR);

// 文字。px から lgfxJapanGothic の段と倍率を選ぶ
const lgfx::IFont* jgFont(int px, float& scaleOut);
void text(const char* s, int x, int y, int px, Color c, textdatum_t datum = middle_center);
void text(const String& s, int x, int y, int px, Color c, textdatum_t datum = middle_center);
int textWidth(const String& s, int px);
String fit(const String& s, int maxWidth, int px);

// 上端のステータス(時刻は RTC が合っているときだけ、電池残量は常に)。値は 10 秒ごとに読み直す
int batteryPercent();
bool batteryCharging();
String timeText();  // RTC が有効なら "HH:MM"、無効なら空
void statusBar(int y = 34);

// KEYA(黄)と KEYB(青)の役割表示。レイアウトは回さず、物理ボタンのある側(通常は上、反転時は下)に出す。nullptr は描かない
void buttonHints(const char* keyA, const char* keyB, float dim = 1.0f);

// リンク種別のアイコン(公式ロゴは使わず頭文字か地球儀)
struct KindStyle {
  Color color;
  Color textColor;
  const char* glyph;  // 空なら label の 1 文字目
  const char* name;   // 表示名
  bool globe;
};
KindStyle kindStyle(const String& kind);
String kindGlyph(const String& kind, const String& label);
void kindIcon(const String& kind, const String& label, int cx, int cy, int r, bool selected);

// アプリのアイコン(記号だけ。背景の円は呼び出し側で描く)
enum class AppIcon { Badge, Clock, Stopwatch, Settings };
Color appColor(AppIcon a);
const char* appName(AppIcon a);
void appIcon(AppIcon a, int cx, int cy, int s, Color fg, Color bg);

// 角丸のダイアログ(中央)。本文 1 行と、KEYA / KEYB の役割を出す
void dialog(const String& body, const char* keyA, const char* keyB);

}  // namespace ui
