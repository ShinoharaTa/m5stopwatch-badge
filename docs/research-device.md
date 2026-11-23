# 調査メモ: M5Stack StopWatch(SKU C152)

2026-11-22 に Web で集めた一次情報のまとめ。各項目の末尾に出典。「未確認」はそのまま残してある。

## 製品

- 正式名称 StopWatch、ストア名 "StopWatch Dev Kit (ESP32-S3)"、SKU C152。公式ドキュメント(英) https://docs.m5stack.com/en/core/StopWatch 、(日) https://docs.m5stack.com/ja/core/StopWatch
- v1.0 リリース 2026-05-22、v1.0.1 リリース 2026-08-27。v1.0 の一部個体は背面シールの "BAT" が実際には 5V IN(公式が「電池をつなぐな」と警告)。
- 海外価格 US$45(CNX Software 2026-05-22 https://www.cnx-software.com/2026/05/22/m5stack-stopwatch-esp32-s3-devkit-offers-1-75-inch-touch-amoled-microphone-speaker-and-gpio-expansion/ )。
- 国内はスイッチサイエンス商品番号 11200、税込 8,283 円、発売 2026-06-25、調査時点で売り切れ(検索抜粋のみ、ページ本体は未取得) https://www.switch-science.com/products/11200
- 出荷時ファームの使い方 https://docs.m5stack.com/en/guide/stopwatch/usage (AlarmClock / WatchFace / Stopwatch / Badge(画像表示)/ IMU / Audio.FFT / LuckyWheel / Settings)

## ハードウェア(出典: 上記公式ドキュメント)

| 項目 | 内容 |
|---|---|
| MCU | ESP32-S3R8(LX7 ×2、240MHz)、Flash 16MB、PSRAM 8MB |
| 無線 | 公式表記は 2.4GHz Wi-Fi。BLE 5.0 は CNX 記事とコミュニティ作例(BLE マウス)から実質あり |
| 表示 | 1.75 インチ円形 AMOLED、466×466、ドライバ CO5300、QSPI(CS=G39 SCK=G40 TE=G38 D0-3=G41/G42/G46/G45)。M5GFX 上のパネルメモリは 480×480 |
| タッチ | CST820B(SDA=G47 SCL=G48 INT=G13) |
| ボタン | プログラマブル 2 個(KEYA 黄=G2、KEYB 青=G1)+ 電源ボタン。物理配置は未確認 |
| 電源ボタン | 1 回押しで ON/リセット、2 回押しで OFF、USB 接続中に長押し(緑 LED 点滅)でダウンロードモード |
| IMU | BMI270(I2C 0x68) |
| RTC | RX8130CE(0x32) |
| 音声 | ES8311 コーデック、MEMS マイク、8Ω 1W スピーカー、アンプ AW8737A |
| 振動 | あり(M5IOE1 の PYG9 を PWM 駆動) |
| 電池 | 内蔵 450mAh。USB-C 充電、既定 185mA(PMG3 LOW で 425mA) |
| 電源 IC | M5Stack 独自の M5PM1 + I/O エキスパンダ M5IOE1(0x4F)。AXP 系ではない |
| 端子 | HY2.0-4P(Port.A: G10/G11)、背面 2.54mm 7P+6P(G3〜G9、G0、3V3、BAT、UART0/USB 切替) |
| 外形 | 52.0×52.0×15.5mm、39g。ストラップ穴と背面マグネット。腕時計バンドの同梱は未確認(首掛け前提の記述) |

## 電源まわり

- 公称の電池持ちは未確認(公式に記載なし)。
- AMOLED なのでバックライトは無く、パネル輝度コマンド 0x51(0〜255)で調光。M5GFX の `Panel_AMOLED::setBrightness()` が送る。 https://github.com/m5stack/M5GFX/blob/master/src/lgfx/v1/panel/Panel_AMOLED.inl
- 電源レベル(https://docs.m5stack.com/en/arduino/stopwatch/m5pm1_m5ioe1 ): L0 = M5PM1 のみ、L1 = +IMU/RTC、L2/L3A = +ESP32-S3/Grove/EXT、L3B = +表示/スピーカー/振動。`pm1.shutdown()` で L0。
- 復帰手段: 電源ボタン、BMI270 割り込み(wrist-wear wake-up / any-motion、M5PM1 の PYG0 経由)、RX8130 のアラーム/タイマー、M5PM1 のタイマー(`timerSet`、POWERON/REBOOT)。ESP32-S3 へは G12(PY_IRQ)で割り込みが届く。RTC 経由の起床例は M5Unified 0.2.21 以降。
- 公式実測(M5StopWatch-PowerDemo、ESP-IDF 5.5.5、3.7V 時の平均入力電流) https://github.com/m5stack/M5StopWatch-PowerDemo

| 状態 | 平均電流 |
|---|---|
| L0 シャットダウン | 8.89µA |
| L1 スタンバイ | 20.49µA |
| L2 ディープスリープ | 90.06µA |
| IMU 起床待ち(L1 低電力設定) | 37.56µA |
| RTC アラーム待ち(L1) | 20.44µA |
| 通常動作 | 45.69mA |
| AMOLED 100% 表示負荷 | 154.45mA |
| 画面 OFF + IMU サンプリング | 36.32mA |
| 画面 OFF + Wi-Fi light sleep | 3.49mA |
| Wi-Fi 接続アイドル | 105.73mA |
| 全負荷 | 285.77mA |

- 常時表示の見積もり(計算値): 通常動作 45mA で 450mAh なら約 10 時間。白背景を多用すると 100mA 超で 4 時間前後。終日の常時表示は無理筋。
- サードパーティ名札ファーム(https://github.com/Ocean-lhy/M5Stopwatch-DIY-Badge )の見積もり: 暗い名刺画面 約 48mA、低輝度待機 約 45mA、QR 画面(高輝度) 約 84mA。同ファームは焼き付き対策として毎分 3〜8 ピクセル画面をずらしている。
- 「CPU 動作のまま画面だけ OFF」の単独実測は未確認。近い値は画面 OFF + Wi-Fi light sleep 3.49mA。

## ソフトウェア

- UiFlow2 対応(電源/RTC/音声/BtnA/BtnB の例、LVGL と日本語フォント)。輝度・スリープ API は未確認。 https://uiflow-micropython.readthedocs.io/en/latest/controllers/stopwatch.html
- Arduino: ボード名 `M5StopWatch`、M5Stack Board Manager 3.3.7 以降。公式表示サンプルの要件は Board Manager ≥3.3.7、M5Unified ≥0.2.15、M5GFX ≥0.2.21。 https://docs.m5stack.com/en/arduino/stopwatch/display
- PlatformIO: 専用ボード定義なし。公式例は `[env:m5stack-stopwatch]` で `platform = espressif32 @ 6.12.0`、`board = esp32s3box`、パーティション `default_16MB.csv`、ライブラリ M5Unified / M5GFX / M5PM1 / M5IOE1。 https://docs.m5stack.com/en/core/StopWatch
- M5GFX / M5Unified: enum `m5::board_t::board_M5StopWatch`(=30) https://github.com/m5stack/M5GFX/blob/master/src/lgfx/boards.hpp 。対応追加は M5GFX 0.2.21(2026-05-15)、M5Unified 0.2.16(2026-05-27)。2026-11 時点の最新はどちらも 0.3.0。要件は「M5Unified 0.2.16 以上」と読むのが安全。
- M5Unified の Power_Class は充電の有効/無効、外部 5V 出力、振動(`cap_set_vibration`)を扱える。
- ESP-IDF: 出荷時ファーム https://github.com/m5stack/M5StopWatch-UserDemo (ESP-IDF v5.5.4、PlatformIO でもビルド可)、省電力例 https://github.com/m5stack/M5StopWatch-PowerDemo 、Arduino 移植の非公式版 https://github.com/Moskic/M5StopWatch-UserDemo-Arduino
- 出荷時 Badge 機能: 端末の AP「M5StopWatch-xxxx」に接続し 192.168.4.1 から画像をアップロード(スマホからは 3MB 未満)。

## 出荷時ファームの画面遷移(2026-11-23 調査)

出典: https://docs.m5stack.com/en/guide/stopwatch/usage と https://github.com/m5stack/M5StopWatch-UserDemo の `main/apps/app_launcher/view/view.cpp`。

- ボタンの物理位置: KEYA(黄)は本体の左上(10〜11 時)、KEYB(青)は右上(1〜2 時)。公式写真で確認。
- ランチャー: 黒背景。1 ページ 1 アプリの横カルーセル。中央に 200×200 のアイコン(y は中心から -15)、左右にタップできるシェブロン、アイコンの下にアプリ名(中央に近づくとフェードイン)、その下にページドット(中心から +200)、上端に時刻。ページ間隔は 466px で中央にスナップ、端は疑似的に無限スクロール。
- 操作: A / B またはシェブロンのタップで左右に移動、アイコンのタップで起動、A+B 同時長押しでランチャーに戻る。上端から下へスワイプで電池残量(ステータスバー)。充電開始時にもステータスバーが出る。
- Badge アプリ: 画像の全画面表示。画面長押しで「Enter badge edit?」の角丸ダイアログ(濃紺のパネルに緑の Edit と灰色の Cancel のピル型ボタン)。Edit で AP `M5StopWatch-xxxx` を立て、192.168.4.1 の Badge Manager(スロット 6 個、Zoom と Background の調整)から画像をアップロード。
- Settings: 輝度、音量、時刻/日付、ファームウェアバージョン。スリープ時間の項目は記載なし。
- 実装: ESP-IDF 5.5.4、Mooncake 2.3.3(アプリ枠組み)+ LVGL 9.5 + smooth_ui_toolkit。アプリは `main/apps/app_*`(alarm_clock / badge / fft / fluidbox / imu / launcher / lucky_wheel / setup / stopwatch / template / watch_face)、共通部品は `apps/common`(status_bar、key_manager、loading_page、arc_top_clock)。HAL は `main/hal`。IMU は `BMI270_BMM150_Sensor` 0.1.2。
- このファームへの示唆: 「A/B で左右、タップで決定、A+B 長押しでホーム」という作法を踏襲すると、標準ファームに慣れた人がそのまま使える。ランチャーの寸法(アイコン 200、名前の位置、ドット +200)も流用できる。

## コミュニティ作例

- 名札 + デジタル名刺 + QR: https://github.com/Ocean-lhy/M5Stopwatch-DIY-Badge (ESP-IDF 5.5。名刺画面、アバター JPEG、黄ボタンで全画面 QR 最大 3 個、低輝度待機、Web 編集、消費電流モデル、焼き付き対策)。今回の用途に最も近い。
- QR サインイン(カンファレンスバッジ): https://workos.com/init/badge 、ファーム https://github.com/chantastic/stopwatch
- 画像を IMU で転がす: https://github.com/NagArgon/M5StopWatch-Drops (M5GFX `pushRotateZoom` の DMA 転送)
- キャラクター表示: https://github.com/nananauno/M5-guruguru
- 複数ファーム共存とランチャー: https://github.com/wararyo/M5StopWatch-MultiFirm 、 https://github.com/wararyo/M5StopWatch-wararyoLauncher
- その他: プレゼンクリッカー https://github.com/3110/M5WatchClicker 、BLE マウス https://github.com/Yeq6X/m5stopwatch-ble-mouse 、イベントカウントダウン https://github.com/denchi-45/M5StopWatch-EventCountDown 、Codex 監視 https://github.com/omiya-bonsai/M5STOPWATCH-CodexMonitor
- 省電力特化のウォッチフェイス、Qiita/Zenn/note の記事は見つからず。

## QR を画面に出す前提(計算値)

- 円形 466px、内接正方形は一辺 約 329px。約 266ppi(0.095mm/px)。

| バージョン(モジュール) | 内接正方形・静止域なし | 内接正方形・4 モジュール静止域込み |
|---|---|---|
| v3(29) | 11px/モジュール(319px) | 8px(コード 232px、静止域込み 296px) |
| v4(33) | 9px(297px) | 8px(264px / 328px) |
| v5(37) | 8px(296px) | 7px(259px / 315px) |
| v6(41) | 8px(328px) | 6px(246px / 294px) |

- 黒地の AMOLED では白い静止域を描く必要がある。円全体を白で塗れば正方形の外側が静止域として働き、「静止域なし」の列まで拡大できる。
- 前述の名札ファームは ECC M、整数ピクセルのモジュール、4 モジュール静止域、QR 表示時は輝度最大。

## 未確認のまま残っている項目

公式ストアとスイッチサイエンスの現在価格・在庫、公称電池持ち、腕時計バンドの有無、ボタンの物理配置、電源ボタンの GPIO、「画面だけ OFF」の単独実測、UiFlow2 の輝度/スリープ API。
