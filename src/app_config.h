// 本体が読む設定(data/config.json)。config.yaml を tools/build_assets.py で変換したもの。
#pragma once
#include <Arduino.h>
#include <FS.h>
#include <vector>

// 輝度の下限。これより暗いと AMOLED で UI が見えなくなる(輝度 0 で UI が消えた不具合の対策)
constexpr uint8_t kMinBrightness = 20;

struct LinkConfig {
  String kind;   // x / bluesky / github / nostr / instagram / threads / web など
  String label;  // QR の下に出す短い文字列(空なら URL から作る)
  String url;    // そのまま QR に入れる
};

struct ProfileConfig {
  String id;
  String name;
  String avatar;        // LittleFS 上のパス(例 /avatars/shino3.jpg)。空なら画像なし
  int avatarSize = 280; // 正方形の一辺(px)
  std::vector<LinkConfig> links;
};

struct DeviceConfig {
  uint8_t brightness = 120;      // 通常時の輝度 0-255
  uint8_t qrBrightness = 255;    // QR 表示中の輝度
  uint32_t dimAfterSec = 20;     // 無操作で減光するまでの秒数(0 で無効)
  uint32_t offAfterSec = 30;     // 無操作で画面 OFF までの秒数(0 で無効)
  uint32_t standbyAfterMin = 5;  // 画面 OFF からスタンバイ(ディープスリープ)までの分数(0 で無効)
  uint32_t qrTimeoutSec = 60;    // QR を閉じ忘れたとき Badge に戻る秒数
  bool wakeOnWrist = true;       // 手首装着検知で点灯(未実装)
  bool pixelShift = true;        // 焼き付き対策のピクセルシフト
  bool flip = true;              // 画面の向き。true = 首掛け(本体を 180° 回した向き、ボタンが下)、false = 手持ち(ボタンが上)
  bool eventModeEnabled = false; // 常時表示(減光・消灯しない)
  uint8_t eventBrightness = 40;
};

struct AppConfig {
  DeviceConfig device;
  std::vector<ProfileConfig> profiles;
  String error;  // 読めなかった理由(成功なら空)

  bool load(fs::FS& fs, const char* path);
};
