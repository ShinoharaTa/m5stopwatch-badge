# M5 StopWatch SNS バッジ 仕様(第 0 版・相談用)

実機の事実は [research-device.md](research-device.md)、各 SNS の URL 形式は [research-sns.md](research-sns.md) にある。ここは「何を作るか」と「既定値として何を決めたか」だけを書く。既定値は最初の確認で変えてよい。

## 目的

- M5Stack StopWatch(C152)を首から下げるか机に置き、普段は自分のアバターを表示しておく。
- 相手に渡したいとき、各 SNS プロフィールへの QR コードを画面に出して読み取ってもらう。
- 表示する SNS とアバターは YAML のリストで管理し、PC 側で変換して本体に書き込む。
- 時計やストップウォッチなど他の機能も同じファームに足せる構造にしておく。
- 電池を 1 日持たせる。常時表示は原則やらず、必要なときだけ点ける。

## 決定事項(既定値)

| 項目 | 既定値 | 理由 |
|---|---|---|
| 開発環境 | PlatformIO + Arduino フレームワーク。`platform = espressif32 @ 6.12.0`、`board = esp32s3box`、`default_16MB.csv`、M5Unified ≥0.2.16、M5GFX ≥0.2.21、M5PM1、M5IOE1 | 公式ドキュメントが示す PlatformIO 構成そのまま。Mac に pio 導入済み、`co2-monitor` と同じ流儀 |
| 設定の持ち方 | 人が編集するのは `config.yaml` だけ。`tools/build_assets.py` が `data/config.json` と画像に変換し、`pio run -t uploadfs` で LittleFS へ書く。本体では YAML を解釈しない | 本体に YAML パーサを載せる価値がない。JSON は ArduinoJson で読める |
| QR の生成 | 本体側で生成。M5GFX 同梱の `lgfx_qrcode_initText()` を ECC M で直接呼び、白い円の上に整数ピクセルのモジュールで自前描画(静止域 4 モジュール以上)。表示中は輝度最大 | 事前レンダリング不要で URL を差し替えるだけで済む。M5GFX の `qrcode()` は ECC L 固定なので使わない |
| アバター | PC 側で円形トリミングした JPEG(既定 384×384、黒背景)に変換して LittleFS へ。本体は `drawJpgFile` で描く | M5GFX が報告する画面は 468×468。384 なら下に 32px の名前を置いても円に収まる |
| 操作 | KEYA(黄)= 決定、KEYB(青)= 戻るに全画面で固定し、物理ボタンの側に役割を表示。Badge → タップか KEYA → リンク一覧(縦の一覧、行のタップで QR)→ QR。Badge で KEYB → ランチャー(縦の一覧)。A+B 長押しでどこからでもランチャー。Settings はトグル・切替・シークバーを直接操作する一覧。常時表示中の Badge はロックし A+B 同時押しで解除 | 案 B のリング UI は実機で操作しにくかった(2026-11-23 の本人フィードバック)。名刺用途では一覧を開いて行を押すのが確実 |
| 電源(通常モード) | 無操作 20 秒で減光、30 秒で画面 OFF、5 分でスタンバイ。スタンバイは ESP32 のディープスリープ(公式 PowerDemo の L2、実測 90.06µA)で、KEYA / KEYB / 電源ボタンで起きて再起動する。M5PM1 の L1(20.49µA)は ESP32 ごと止まるので KEYA/KEYB では起きられず、手首装着検知(PM1 GPIO0 経由)を足す段階で選べるようにする | 通常動作 45mA なので点けっぱなしは 10 時間が上限。90µA でも 450mAh なら自己放電が支配的で、差より「どのボタンでも起きる」ほうが効く |
| イベントモード | 任意で有効化。黒背景・低輝度でアバターを表示し続ける。見積 45〜50mA、450mAh で 8〜9 時間。1 分ごとに数ピクセルずらす | 1 日のイベントなら朝満充電で足りる見込み。焼き付き対策は必須 |
| 焼き付き対策 | 全画面で黒背景基調、静止画はピクセルシフト、QR は閉じ忘れ防止で 60 秒で自動復帰 | AMOLED |
| 機能の構造 | 画面(Scene)単位のアプリ構造。第 1 段階は Badge / QR / Settings。第 2 段階で Clock / Stopwatch / Web 設定 | 「いろいろな機能を入れたい」に備える |
| 複数ファーム共存 | 第 1 段階では考えない。wararyo の MultiFirm と共存させたくなったらパーティションを合わせる | まず単体で動かす |

## 画面と操作(第 1 段階)

```
[Badge: アバター]  ←スワイプ→  [Badge: 次の項目] ...
      │ KEYA
      ▼
[QR 選択メニュー]  (タップかスワイプで選び KEYA で決定 / KEYB で戻る)
      │
      ▼
[QR 全画面]  (KEYB か 60 秒で Badge に戻る)
```

- Badge 画面: アバター(円形)、下に表示名。項目(プロファイル)が複数あればスワイプで切り替わる。
- QR 選択メニュー: そのプロファイルの `links` をアイコンと名前で一覧。
- QR 全画面: 白い円の上に QR、下に小さくラベル(例: `x.com/SHINOHARATTT`)。
- Settings: 輝度、消灯までの秒数、イベントモードの ON/OFF、電池残量。
- 画面 OFF からの点灯は、まずボタンとタップを確実にし、手首装着検知は動けば足す。

## 設定ファイル

`config.example.yaml` に例がある。要点:

```yaml
device:
  brightness: 120        # 通常時 0-255
  dim_after_sec: 20
  off_after_sec: 30
  standby_after_min: 5
  event_mode: { enabled: false, brightness: 40 }
profiles:
  - id: shino3
    name: shino3
    avatar: assets/avatar-shino3.jpg
    links:
      - { kind: x, url: "https://x.com/SHINOHARATTT" }
      - { kind: bluesky, url: "https://bsky.app/profile/shino3.net" }
```

- `kind` はアイコンと既定ラベルを引くためのキー(x / bluesky / github / nostr / instagram / threads / web など)。未知の `kind` は汎用アイコンで表示する。
- `url` はそのまま QR に入れる(バイトモード、ECC M。30 文字前後なら V3、80 文字でも V5)。`nostr` だけは一般向けの Web ゲートウェイ URL を既定にし、Nostr ユーザー向けに `nostr:npub…` へ切り替えられるようにする。

## 電源設計の根拠

M5StopWatch-PowerDemo の公式実測(3.7V)を使う。詳細は research-device.md。

| 状態 | 電流 | 450mAh での持ち時間(計算) |
|---|---|---|
| 通常動作(画面あり) | 45.7mA | 約 10 時間 |
| AMOLED 100% 表示 | 154mA | 約 3 時間 |
| 画面 OFF + Wi-Fi light sleep | 3.5mA | 約 5 日 |
| L1 スタンバイ | 20µA | 実質電池の自己放電が支配 |

## 初期データ

- アバター: `assets/avatar-shino3.jpg`(240×240、shino3.net のアイコン)。428lab 用は `assets/428lab-icon.svg`(SVG なので PNG 化が必要)。
- SNS リンク候補(shino3.net と 428lab.net のサイトから拾ったもの。最終リストは本人に確認):
  - 個人: X `https://x.com/SHINOHARATTT`、Bluesky `https://bsky.app/profile/shino3.net`、GitHub `https://github.com/ShinoharaTa`、Nostr `npub1l60d6h2uvdwa9yq0r7r2suhgrnsadcst6nsx2j03xwhxhu2cjyascejxe5`(サイトでは `https://nostx.io/npub…` を使用)、Instagram `https://www.instagram.com/shinoharattt/`、Threads `https://www.threads.com/@shinoharattt`(2025-04 に threads.net から移行済み)、Web `https://shino3.net`
  - 四谷ラボ: X `https://x.com/lab_428`、Bluesky `https://bsky.app/profile/428lab.net`、GitHub `https://github.com/428lab`、Web `https://428lab.net`(2026-11-22 時点で DNS がパーキングに向いている。復旧は別件)

## やらないこと(第 1 段階)

- Wi-Fi 経由の設定アップロード、OTA、Web UI
- 本体側での YAML 解釈
- 時計・ストップウォッチ・アラーム(第 2 段階)
- 音声・マイク・スピーカー
- 複数ファーム共存

## 未決事項(最初に本人へ確認)

1. 実機は手元にあるか。背面シールの版(v1.0 / v1.0.1)。腕時計バンドの有無(首掛けか手首か)。
2. 表示する SNS の最終リストと並び順。候補は上の「初期データ」。
3. 個人と四谷ラボを別プロファイルとしてスワイプで切り替えるか、1 プロファイルに全部入れるか。
4. イベントモード(常時表示)を第 1 段階に入れるか。
5. 第 2 段階に何を入れたいか(時計、ストップウォッチ、カウントダウン、ほか)。
6. PlatformIO + Arduino で M5PM1 の省電力 API(L1 スタンバイ、IMU 起床、RTC 起床)が使えるかの確認結果次第で、電源まわりだけ ESP-IDF 化するか。(2026-11-23 のソース確認: 公式 Arduino ライブラリ M5PM1 1.0.7 に `begin(m5::I2C_Class*)` があり M5Unified の I2C を共有できる。`gpioSetWakeEnable` / `gpioSetWakeEdge` / `timerSet` / `setDcdcEnable` / `setLdoEnable` がそろっているので、Arduino のまま L1 相当に入れる見込み。実機で未検証)

## 第 2 段階の候補

- Clock(RTC、画面 OFF 中も RTC で時刻維持)、Stopwatch、Countdown
- Web UI(AP モード)で `config.json` とアバターを差し替え
- Badge 画面でのアニメーション、IMU 連動
- 複数ファーム共存(MultiFirm)

## 決めた経緯

- 2026-11-22: 本人の要望は「普段アバター、必要なら SNS の QR、設定は YAML のリスト、左右スワイプで切替、ボタンで QR 選択、他の機能も入れたい、電池持ちが気になる、常時表示は難しそう」。実機調査で通常 45mA / 待機 20µA が分かったので、常時表示は任意のイベントモードに限定し、既定はスリープ主体とした。
- 2026-11-22: 確認済みの事実。M5GFX の `LGFXBase.hpp` に `qrcode(const char*, x, y, width, version, bool margin)` がある。PlatformIO レジストリに `m5stack/M5PM1` 1.0.7、`m5stack/M5IOE1` 1.0.9、`m5stack/M5Unified` 0.3.0 がある(2026-11-22 の `pio pkg search`)。Mac の PlatformIO は 6.1 系で 6.2.0 への更新が案内されている。
- 2026-11-23: 第 1 マイルストーンのコードを書きビルドが通った(`platformio.ini`、Badge / QR 画面、減光 → 消灯 → 復帰、`tools/build_assets.py`)。実機未確認。確認したライブラリの事実: M5Unified 0.3.0 は StopWatch を自動判別し、KEYA(G2)= `BtnA`、KEYB(G1)= `BtnB`、タッチは M5GFX が CST820 / CST9217 を判別、電源 IC は M5Unified 内蔵の `M5PM1_Class`(`M5.Power.M5pm1`)が扱う。画面は M5GFX 上で 468×468(オフセット x=6)。`M5.Display.sleep()` / `wakeup()` で AMOLED を止めて戻せる。M5GFX の `qrcode()` は ECC L 固定だったので QR は自前描画にした。
- 2026-11-23: 実機で第 1 段階を確認。起動 1.2 秒で Badge、減光 21 秒、消灯 31 秒。IMU(BMI270)の重力方向にアバターを滑らせる「傾き連動」を試作したが、本人が実機で「角度に対する動きが期待と違って気持ち悪い」と判断し不採用。Badge は静止表示(ピクセルシフトのみ)とする。IMU は手首装着検知での点灯にだけ使う予定。
- 2026-11-23: 出荷時ファームの遷移を調査(docs/research-device.md)。ランチャーは 1 ページ 1 アプリの横カルーセル、A/B で左右、タップで起動、A+B 長押しでホーム。機能追加に備え、このファームも同じ作法のランチャーを持つ方向で画面デザイン案(docs/design/)を作って本人に選んでもらう。
- 2026-11-23: 画面デザインは 3 案(docs/design/)から本人が案 B「リング・ダイヤル」を選択。全画面を PSRAM のフレームバッファに描いて一括転送する方式で実装し、ランチャー(Badge / Clock / Stopwatch / Settings)、リンク選択、QR、Settings(NVS 保存)、復帰時の電池表示まで入れた。Clock と Stopwatch は置き場だけ。
- 2026-11-23: 歩くときは首から下げるので本体が上下逆になる、との本人の指摘。画面の向きを設定 `flip`(既定 true = 首掛け、180° 回転)にし、Badge の KEYA 長押しと Settings の「画面の向き」で切り替えられるようにした(NVS に保存)。回転は `M5.Display.setRotation(2)` で行いタッチ座標も一緒に回る。本人の指摘で、レイアウト(上にステータス、下に名前)は向きによらず同じにし、ボタンの役割表示だけを物理ボタンのある側(首掛けでは下の両隅、KEYA = 右下、KEYB = 左下)に出すようにした。
- 2026-11-23: スタンバイを実装。公式 PowerDemo の L2 手順(BMI270 を PWR_CONF=0x01 → PWR_CTRL=0x00 でサスペンド、CST820 に 0xE5=0x03、IOE1 の表示/コーデック/アンプ/振動をハイインピーダンス、PM1 のブーストと LED を OFF、IOE1 0x23 と PM1 0x09 の下位 4 ビットで I2C アイドルスリープ 1 秒、1 秒待って `esp_deep_sleep_start()`)を M5Unified の I2C で直接書いて再現。起床は ext1(G1/G2 のどれかが Low)。復帰後は `rtc_gpio_deinit` でボタンを通常 GPIO に戻し、起床に使った押下は画面に渡さない。L1 は見送り(理由は決定事項の表)。
- 2026-11-23 朝: スタンバイを実機で確認。消灯の 1 分後(テスト設定)にディープスリープへ入り USB が消え、KEYA で起きて `wake: deep sleep → KEYA/KEYB`、約 1 秒で Badge。設定は 5 分に戻した。
- 2026-11-23 夜: 本人が 1 日使ったフィードバックで UX を直した(v0.3.0)。
  - Settings で値が今の値から始まらず、確定すると輝度 0 になって UI が消えたのはバグだった(編集開始時に前回の編集値を読んでいた)。Settings を編集モードのない一覧(トグル、首掛け/手持ちの切替、シークバー)に作り直し、輝度の下限を 20 にした(NVS に残った 0 も起動時に丸める)。
  - メニュー(ランチャー、リンク一覧、Settings)は入ったときの輝度(最低 80)で一定にし、減光しない。Settings で変えた輝度は出たときに反映する。
  - 「イベント時の輝度」を Settings のシークバーに追加。
  - 常時表示中の Badge はロックし、A+B 同時押しで解除、無操作 15 秒で再ロック。
  - メニュー画面の上端に電池残量(RTC が合っていれば時刻)を出す。
  - リンク選択とランチャーのリング UI をやめ、中央の行が選択中の縦一覧にした。行のタップで開く。Badge のタップでもリンク一覧を開く。
- 2026-11-24: v0.3.0 を実機に書き込み、本人が一覧・Settings・常時表示のロックを試して「問題なし」。ログでも Settings の各操作が 1 回ごとに保存され、エラーや再起動はなかった。
