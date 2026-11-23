// 画面ごとの輝度と、無操作時の減光 → 画面 OFF → スタンバイ。
//   Badge: 通常は輝度 brightness、減光と消灯あり。イベントモードは event_brightness で常時表示
//   Menu(ランチャー、リンク、Settings など): 入った時点の輝度で一定(最低 80)、減光しない。
//         無操作 max(消灯までの秒, 60 秒)で、通常は消灯、イベントモードは Badge に戻る
//   Qr: qr_brightness、減光も消灯もしない(QR 側の 60 秒タイムアウトで Badge へ)
#pragma once
#include <Arduino.h>

#include "app_config.h"

enum class ScreenState { On, Dim, Off };
enum class ScreenPolicy { Badge, Menu, Qr };

class PowerManager {
 public:
  enum class Action { None, GoBadge };
  enum class Wake { None, FromDim, FromOff };

  void begin(DeviceConfig* dev);
  void setPolicy(ScreenPolicy p);  // 画面を切り替えるたびに呼ぶ
  void refresh();                  // 設定を変えたら呼ぶ(Menu 中は輝度を変えない)

  // 操作があったときに呼ぶ。減光や消灯から戻したなら、その操作は明るくするのに使ったので画面には渡さない
  Wake noteActivity(uint32_t now);
  Action tick(uint32_t now);
  void screenOff(uint32_t now);
  ScreenState state() const { return _state; }
  uint32_t idleMs(uint32_t now) const;

 private:
  uint8_t target() const;
  void applyBrightness();
  void setState(ScreenState s, uint32_t now);

  DeviceConfig* _dev = nullptr;
  ScreenState _state = ScreenState::On;
  ScreenPolicy _policy = ScreenPolicy::Badge;
  uint8_t _menuBrightness = 120;
  uint32_t _lastActivity = 0;
  uint32_t _offSince = 0;
};
