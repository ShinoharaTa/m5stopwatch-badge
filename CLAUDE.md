# m5stopwatch-badge

M5Stack StopWatch(SKU C152、ESP32-S3、1.75 インチ円形 AMOLED 466×466、タッチ、ボタン 2 個、450mAh)用のファームウェア。
普段はアバターを表示し、ボタンで各 SNS プロフィールの QR コードを出す。設定は YAML のリストで管理する。
他の機能(時計など)も同じファームに足していく。

## まず読むもの

- [docs/spec.md](docs/spec.md): 目的、既定値、画面構成、未決事項
- [docs/research-device.md](docs/research-device.md): 実機の仕様、消費電流の公式実測、開発環境、作例
- [docs/research-sns.md](docs/research-sns.md): 各 SNS のプロフィール URL 形式と QR の注意点
- [config.example.yaml](config.example.yaml): 設定ファイルの形

## 約束事

- 会話・コメント・コミットメッセージは日本語。
- ビルドは PlatformIO。`pio run -e m5stack-stopwatch`、書き込み `-t upload`、LittleFS は `-t uploadfs`。ボード定義は公式例のとおり `esp32s3box` を借りる。
- 書き込みに失敗したら、USB 接続のまま電源ボタン長押し(緑 LED 点滅)でダウンロードモードに入る手順を案内する。
- 本体の画面はスクリーンショットが撮れない。見た目の確認は本人に写真をもらう。レイアウトの机上確認が要るなら、PC 側で同じ描画ロジックから PNG を出す小さなツールを作ってよい。
- 設定は `config.yaml` → `tools/build_assets.py` → `data/`。`config.yaml` と `data/` はコミットしない(`config.example.yaml` を更新して形を残す)。
- 電源まわりの判断は docs/research-device.md の公式実測値を根拠にして書く。推測で数字を出さない。
- AMOLED なので、全画面で黒背景基調とピクセルシフトを守る。白べた塗りは QR 表示のときだけ。
- QR は本体で生成する(M5GFX の `qrcode()`)。URL は config の値をそのまま入れる。
- 秘密情報(Wi-Fi パスワード等)は `config.yaml` か `.dev.vars` 相当に置き、コミットしない。
- `inbox/` は本人から受け取った写真や素材の置き場。コミットしない。
- コミット・push は頼まれたときだけ。ブランチ→PR→マージの流儀は本人に合わせる。
- 1 つの変更は小さく。ビルドが通ってから次へ進む。

## 最初の指示

1. 上の 4 つのファイルを読む。
2. `pio pkg search` 等で M5Unified / M5GFX の最新と、M5PM1 / M5IOE1 の Arduino ライブラリの入手先(PlatformIO レジストリにあるか、GitHub 直指定か)を確認する。
3. docs/spec.md の「未決事項」を番号付きで本人に聞く。各項目に既定値を添え、返事がない項目は既定値で進める。
4. 実機が手元にあれば、最初のマイルストーンは「`platformio.ini` を作り、画面に文字とアバターを出し、KEYA/KEYB とタッチが拾え、輝度を変えられ、画面 OFF → ボタンで復帰できる」こと。ここまでを 1 本目の PR 相当にする。
5. 次に QR 全画面、QR 選択メニュー、スワイプ切替、`tools/build_assets.py`、Settings、電源管理(減光 → OFF → L1 スタンバイ)の順に進める。
6. 実機がまだ無ければ、`platformio.ini` と変換スクリプトとビルドが通るところまで進め、実機待ちの項目を明記して止まる。
