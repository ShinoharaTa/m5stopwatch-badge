// m5stopwatch-badge: M5Stack StopWatch を SNS 名刺にするファーム。
// 起動直後は Badge。KEYA = 決定、KEYB = 戻る、A+B 長押し = ランチャー。
#include "ui.h"  // LittleFS.h → M5Unified.h の順を守る

#include "app_config.h"
#include "badge_scene.h"
#include "input.h"
#include "launcher_scene.h"
#include "links_scene.h"
#include "placeholder_scene.h"
#include "power_manager.h"
#include "qr_scene.h"
#include "scene.h"
#include "settings_scene.h"
#include "settings_store.h"
#include "standby.h"

namespace {
AppConfig g_config;
PowerManager g_power;
SettingsStore g_store;
AppContext g_ctx;
LauncherScene g_launcher;
BadgeScene g_badge;
LinksScene g_links;
QrScene g_qr;
SettingsScene g_settings;
PlaceholderScene g_clock(ui::AppIcon::Clock);
PlaceholderScene g_stopwatch(ui::AppIcon::Stopwatch);
Scene* g_scene = nullptr;
SceneId g_sceneId = SceneId::Badge;
bool g_configOk = false;
DeviceConfig g_defaults;  // config.json の値(NVS で上書きする前)
bool g_ignoreUntilRelease = false;  // 点灯やスタンバイ復帰に使った操作は画面に渡さない

Scene* sceneFor(SceneId id) {
  switch (id) {
    case SceneId::Launcher: return &g_launcher;
    case SceneId::Badge: return &g_badge;
    case SceneId::Links: return &g_links;
    case SceneId::Qr: return &g_qr;
    case SceneId::Settings: return &g_settings;
    case SceneId::Clock: return &g_clock;
    case SceneId::Stopwatch: return &g_stopwatch;
    default: return nullptr;
  }
}

ScreenPolicy policyFor(SceneId id) {
  switch (id) {
    case SceneId::Badge: return ScreenPolicy::Badge;
    case SceneId::Qr: return ScreenPolicy::Qr;
    default: return ScreenPolicy::Menu;
  }
}

void switchScene(SceneId id) {
  if (id == SceneId::Same) return;
  Scene* next = sceneFor(id);
  if (!next) return;
  if (g_scene) g_scene->leave(g_ctx);
  g_sceneId = id;
  g_scene = next;
  g_power.setPolicy(policyFor(id));
  g_scene->enter(g_ctx);
}

void drawErrorScreen(const char* title, const String& detail) {
  using namespace ui;
  clear();
  text(title, kCx, kCy - 30, 28, kText);
  text(fit(detail, chordWidth(kCy + 10) - 32, 16), kCx, kCy + 10, 16, kTextSub);
  text("pio run -t uploadfs", kCx, kCy + 40, 16, kTextSub);
  present();
}
}  // namespace

void setup() {
  log_i("[boot] setup start v%s", APP_VERSION);
  standby::prepareAfterBoot();
  auto cfg = M5.config();
  cfg.internal_spk = false;  // 音は使わない
  cfg.internal_mic = false;
  cfg.clear_display = true;
  M5.begin(cfg);
  input::begin();
  g_power.begin(&g_config.device);  // どのエラー経路でも loop が電源管理を触れるように先に初期化
  log_i("[boot] wake: %s", standby::wakeReason());
  log_i("[boot] M5.begin done: board=%d display=%dx%d touch=%d imu=%d bat=%d%%", static_cast<int>(M5.getBoard()),
        M5.Display.width(), M5.Display.height(), M5.Touch.isEnabled() ? 1 : 0, static_cast<int>(M5.Imu.getType()),
        static_cast<int>(M5.Power.getBatteryLevel()));

  if (!ui::initFrame()) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("PSRAM error", M5.Display.width() / 2, M5.Display.height() / 2);
    return;
  }

  if (!LittleFS.begin(false)) {
    log_e("[boot] LittleFS をマウントできない");
    g_power.begin(&g_config.device);
    drawErrorScreen("LittleFS が開けません", "data/ を書き込んでください");
    return;
  }
  if (!g_config.load(LittleFS, "/config.json")) {
    log_e("[boot] 設定エラー: %s", g_config.error.c_str());
    g_power.begin(&g_config.device);
    drawErrorScreen("設定が読めません", g_config.error);
    return;
  }
  g_defaults = g_config.device;
  g_store.load(g_config.device);  // 本体で変えた設定があれば上書き
  ui::applyFlip(g_config.device.flip);
  log_i("[boot] profiles=%u brightness=%u dim=%lus off=%lus event=%d", static_cast<unsigned>(g_config.profiles.size()),
        g_config.device.brightness, static_cast<unsigned long>(g_config.device.dimAfterSec),
        static_cast<unsigned long>(g_config.device.offAfterSec), g_config.device.eventModeEnabled ? 1 : 0);

  g_configOk = true;
  g_ctx.cfg = &g_config;
  g_ctx.power = &g_power;
  g_ctx.store = &g_store;
  g_ctx.defaults = &g_defaults;
  g_ctx.justWoke = true;                          // 起動時も電池残量を 2 秒出す
  g_ctx.locked = g_config.device.eventModeEnabled;  // 常時表示ならロックから始める
  g_ignoreUntilRelease = standby::wokeByButton();   // スタンバイ復帰に使ったボタンは渡さない
  g_power.begin(&g_config.device);
  switchScene(SceneId::Badge);
  log_i("[boot] badge shown");
}

void loop() {
  M5.update();
  const uint32_t now = millis();
  const input::Frame in = input::poll(now);

  if (in.pressStarted || in.count > 0) {
    const auto wake = g_power.noteActivity(now);
    if (wake != PowerManager::Wake::None && g_configOk) {
      g_ignoreUntilRelease = true;  // 明るくするのに使った操作は画面に渡さない
      if (wake == PowerManager::Wake::FromOff) {
        g_ctx.justWoke = true;  // 消灯から戻ったら Badge を出し直す(電池残量つき)
        switchScene(SceneId::Badge);
      }
    }
  }

  if (g_configOk && g_scene && !g_ignoreUntilRelease) {
    for (int i = 0; i < in.count; ++i) {
      const InputEvent& ev = in.events[i];
      if (ev.type == InputEvent::GoHome) {
        switchScene(SceneId::Launcher);
        continue;
      }
      switchScene(g_scene->onInput(g_ctx, ev));
    }
  }
  if (g_ignoreUntilRelease && !in.anyPressed) g_ignoreUntilRelease = false;

  if (g_power.tick(now) == PowerManager::Action::GoBadge && g_configOk) {
    g_ctx.locked = true;  // 常時表示でメニューを放置したら Badge に戻してロック
    switchScene(SceneId::Badge);
  }
  // 消灯中は画面側の更新を止める
  if (g_configOk && g_scene && g_power.state() != ScreenState::Off) switchScene(g_scene->tick(g_ctx, now));

  M5.delay(g_power.state() == ScreenState::Off ? 50 : 10);
}
