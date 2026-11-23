// スタンバイ(ESP32 のディープスリープ = 公式 PowerDemo の L2 相当、実測 90µA)。
// 画面 OFF から standby_after_min 経過で入り、KEYA / KEYB(または電源ボタン)で起きて再起動する。
// PM1 を落とす L1(20µA)は ESP32 ごと止まるので KEYA/KEYB では起きられず、手首装着検知を足すときの選択肢にする。
#pragma once

namespace standby {

// setup() の最初(M5.begin の前)で呼ぶ。ディープスリープ明けの RTC IO を通常の GPIO に戻し、起床理由を記録する
void prepareAfterBoot();

// ディープスリープから KEYA / KEYB で起きたか(その押下は画面に渡さない)
bool wokeByButton();

// 起床理由の短い説明(ログ用)
const char* wakeReason();

// スタンバイに入る(戻らない)
void enter();

}  // namespace standby
