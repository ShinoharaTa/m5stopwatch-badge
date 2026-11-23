# m5stopwatch-badge

M5Stack StopWatch(SKU C152)を SNS 名刺にするファームウェア。普段はアバターを表示し、KEYA(黄)で各 SNS プロフィールの QR コードを全画面に出す。表示する SNS とアバターは `config.yaml` で管理する。

仕様は [docs/spec.md](docs/spec.md)、実機の調査は [docs/research-device.md](docs/research-device.md)、各 SNS の URL 形式は [docs/research-sns.md](docs/research-sns.md)。

## 使い方

```bash
# 1. 設定を書く(config.yaml はコミットしない)
cp config.example.yaml config.yaml

# 2. 変換ツールの Python 環境(初回だけ)
python3 -m venv .venv && .venv/bin/pip install -r tools/requirements.txt
brew install librsvg   # SVG のアバターを使うときだけ

# 3. config.yaml → data/(config.json とアバター画像)
.venv/bin/python tools/build_assets.py

# 4. ビルドと書き込み(PlatformIO)
pio run -e m5stack-stopwatch -t upload     # ファーム
pio run -e m5stack-stopwatch -t uploadfs   # data/ を LittleFS へ
pio device monitor -b 115200
```

書き込みに失敗したら、USB を挿したまま電源ボタンを長押し(緑 LED 点滅)でダウンロードモードに入れてからやり直す。

## 操作

KEYA(黄)= 決定、KEYB(青)= 戻る。KEYA + KEYB を 0.8 秒押し続けると、どこからでもランチャー。起動直後は Badge。

画面の向きは既定で「首掛け」(本体を 180° 回した向き、ボタンが下側)。手に持って使うときは Badge で KEYA を長押しするか、Settings の「画面の向き」で「手持ち」に切り替える。レイアウト(上にステータス、下に名前)はどちらの向きでも同じで、ボタンの役割表示だけが物理ボタンのある側に出る。

| 画面 | KEYA | KEYB | 画面の操作 | 長押し |
|---|---|---|---|---|
| Badge | リンク一覧へ | ランチャーへ | タップ: リンク一覧 / 左右スワイプ: プロファイル切替 | 画面: イベントモード ON/OFF / KEYA: 向きの切替 / KEYB: 画面 OFF |
| リンク一覧 | 中央の行の QR | Badge へ | 縦ドラッグ: スクロール / 行のタップ: その QR | なし |
| QR | 次のリンク | リンク一覧へ | 左右スワイプ: 前後のリンク | なし(60 秒で Badge へ) |
| ランチャー | 中央の行を起動 | Badge へ | 縦ドラッグ: スクロール / 行のタップ: 起動 | なし |
| Settings | なし | 戻る(ここで保存) | 縦ドラッグ: スクロール / トグル・切替: タップ / シークバー: 横ドラッグかタップ | なし |

Settings の項目: 画面の向き、輝度、イベントモード、イベント時の輝度、ピクセルシフト、減光まで、消灯まで、スタンバイまで、電池、バージョン。変えた値は NVS に保存され、`config.yaml` の値より優先される。輝度の下限は 20。

### 常時表示(イベントモード)

- Badge を表示し続ける(輝度は「イベント時の輝度」)。外輪とボタン表示は消し、アバターと名前だけにする。
- 首から下げていて画面に触れても切り替わらないよう、Badge はロックされる。触ると「A+B 同時押しで解除」と出る。KEYA と KEYB を同時に押すと解除、無操作 15 秒でまたロック。
- メニューを 60 秒放置すると Badge に戻ってロックする。

### 電源

- Badge: 無操作 20 秒で減光、30 秒で画面 OFF、画面 OFF から 5 分でスタンバイ(ESP32 のディープスリープ。公式 PowerDemo の L2 相当で実測 90µA)。スタンバイからは KEYA / KEYB か電源ボタンで起きて再起動する(約 1 秒)。
- メニュー(ランチャー、リンク一覧、Settings): 入ったときの輝度(最低 80)で一定にし、減光しない。無操作で max(消灯までの秒, 60 秒) たつと画面 OFF。Settings で変えた輝度は Settings を出たときに反映する。
- QR: 最大輝度(`qr_brightness`)、60 秒で Badge へ。
- 消灯中はどのボタンでもタッチでも点灯し、その操作は画面に渡さない。点灯直後と起動直後は 2 秒だけアバターの外輪が電池残量になる。メニュー画面は上端に電池残量(RTC が合っていれば時刻も)を出す。

## 構成

- `src/main.cpp`: 起動、Scene の切替、画面ごとの電源方針の適用
- `src/input.*`: ボタンとタッチからタップ・長押し・ドラッグ・スワイプ・A+B 同時押しを作る
- `src/ui.*`: フレームバッファ(PSRAM、468×468 を一括転送)、色、極座標と円弧、文字、ボタン表示、ステータスバー、アイコン、ダイアログ
- `src/list_wheel.*`: 縦の一覧(中央の行が選択中、ドラッグでスクロールして吸い付く、行のタップで起動)
- `src/badge_scene.*` / `links_scene.*` / `qr_scene.*` / `launcher_scene.*` / `settings_scene.*` / `placeholder_scene.*`: 各画面
- `src/power_manager.*`: 画面ごとの輝度方針(Badge / メニュー / QR)と、減光 → 消灯 → スタンバイの判定
- `src/standby.*`: スタンバイ(タッチと IMU を眠らせ、IOE1 の負荷を初期状態に戻し、PM1/IOE1 の I2C スリープを有効にしてディープスリープ。KEYA/KEYB で起床)
- `src/settings_store.*`: 本体で変えた設定の NVS 保存
- `src/app_config.*`: `data/config.json` の読み込み(ArduinoJson)
- `tools/build_assets.py`: `config.yaml` → `data/`(アバターは円形 280px、黒背景)
- `docs/design/`: 画面デザイン案(A / B / C)とモック生成スクリプト

## ライセンス

ソースコードとドキュメントは [MIT License](LICENSE)。

`assets/` の画像(アバターと四谷ラボのロゴ)と、それを使った `docs/design/` のモック画像は MIT の対象外。自分の名刺にするときは `config.yaml` で自分の画像に差し替える。
