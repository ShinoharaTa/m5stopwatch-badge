// 画面(Scene)の共通部分。
#pragma once
#include <Arduino.h>

#include "app_config.h"

class PowerManager;
class SettingsStore;

enum class SceneId { Same, Launcher, Badge, Links, Qr, Settings, Clock, Stopwatch };

struct InputEvent {
  enum Type {
    None,
    KeyAClick,   // KEYA(黄)短押し = 決定
    KeyBClick,   // KEYB(青)短押し = 戻る
    KeyAHold,    // KEYA 長押し(Badge: 画面の向きを切替)
    KeyBHold,    // KEYB 長押し(Badge: 画面 OFF)
    BothPress,   // KEYA と KEYB が同時に押された瞬間(常時表示のロック解除)
    GoHome,      // KEYA + KEYB を 0.8 秒押し続けた(どこからでもランチャー)
    Tap,         // 画面を軽く叩いた(x, y)
    TouchHold,   // 画面の長押し(x, y)
    DragStart,   // ドラッグ開始(x, y = 押し始めの位置、dx, dy = 判定時点の移動量、vertical = 縦か)
    DragMove,    // ドラッグ中(x, y = 今の位置、dx, dy = 前フレームからの移動)
    DragEnd,     // ドラッグ終了(dx, dy = 総移動量)
    SwipeLeft,   // 横ドラッグが左へ 50px 以上(次へ)
    SwipeRight,  // 横ドラッグが右へ 50px 以上(前へ)
  };
  Type type = None;
  int x = 0;
  int y = 0;
  int dx = 0;
  int dy = 0;
  bool vertical = false;
};

struct AppContext {
  AppConfig* cfg = nullptr;
  PowerManager* power = nullptr;
  SettingsStore* store = nullptr;
  const DeviceConfig* defaults = nullptr;  // config.json の値(本体の変更を保存するときの基準)
  int profileIndex = 0;
  int linkIndex = 0;
  bool justWoke = false;  // 消灯から戻った直後(Badge が電池残量を出す)
  bool locked = false;    // 常時表示(イベントモード)中の Badge のロック。A+B 同時押しで解除

  const ProfileConfig& profile() const { return cfg->profiles[profileIndex]; }
};

class Scene {
 public:
  virtual ~Scene() = default;
  virtual void enter(AppContext& ctx) = 0;  // 全画面を描く
  virtual void leave(AppContext& ctx) { (void)ctx; }
  virtual SceneId tick(AppContext& ctx, uint32_t now) { (void)ctx; (void)now; return SceneId::Same; }
  virtual SceneId onInput(AppContext& ctx, const InputEvent& ev) { (void)ctx; (void)ev; return SceneId::Same; }
};
