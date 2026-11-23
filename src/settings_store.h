// 本体で変えた設定を NVS に残し、起動時に config.json の値へ上書きする。
#pragma once
#include "app_config.h"

class SettingsStore {
 public:
  void load(DeviceConfig& dev);        // 保存があれば dev を上書き
  // defaults(config.json の値)と違う項目だけ保存し、同じになった項目は消す。
  // こうしておくと、本体で触っていない項目は config.yaml の変更がそのまま効く
  void save(const DeviceConfig& dev, const DeviceConfig& defaults);
  void clear();  // 保存を全部消す(config.json の値に戻す)
};
