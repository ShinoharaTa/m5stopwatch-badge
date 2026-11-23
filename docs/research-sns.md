# 調査メモ: SNS プロフィール URL と QR の形式

2026-11-22 に Web 検索、公式ページ取得、curl での実地確認(リダイレクトと `/.well-known/apple-app-site-association` = AASA の有無)で集めた。
「アプリで開く」は AASA の宣言から判断した見込みで、実機カメラでの挙動はすべて未確認。

## 一覧

| サービス | QR に入れる正規形式 | 文字数の目安 | 実機アプリ連携(AASA) | 備考 |
|---|---|---|---|---|
| X | `https://x.com/<username>` | 14+ユーザー名(最大 29) | 公式アプリが宣言 | twitter.com は 2024-05 に x.com へ移行、今も 301 転送されるが x.com を使う。ユーザー名は英数字と `_`、最大 15 文字。 https://help.x.com/en/managing-your-account/x-username-rules |
| Bluesky | `https://bsky.app/profile/<handle>` または `/profile/<DID>` | ハンドル依存、DID なら約 57 | 公式アプリが全パス | ハンドルは変更できるので恒久掲示なら DID 形式が安全。独自ドメインハンドルを使い続けるならハンドル形式でよい。パスの大文字化は 404。 https://atproto.com/specs/handle |
| Mastodon | `https://<instance>/@<username>` | 25+(mastodon.social の場合) | インスタンス次第(mastodon.social は公式/Ivory が `/@*`) | https://docs.joinmastodon.org/entities/Account/ |
| Misskey | `https://<server>/@<username>` | 20+(misskey.io) | AASA なし、ブラウザ | 公式ドキュメントに明記なし、実測 200 |
| Nostr | `nostr:npub1…`(69 文字)か `https://njump.me/npub1…`(80 文字) | 69 / 80 | njump.me は AASA なし。primal.net は Primal アプリが `/*` | NIP-21 は `nostr:` + NIP-19 識別子。Damus は QR に `nostr:npub`、Amethyst は `nostr:nprofile`(リレーヒント付き、可変長)。一般の人が標準カメラで読むなら https の Web ゲートウェイが無難。bech32 は大文字化できるが Damus の接頭辞判定が小文字前提なので大文字化しない。 https://github.com/nostr-protocol/nips/blob/master/21.md |
| Threads | `https://www.threads.com/@<username>` | 25+ | Threads アプリが宣言 | 2025-04-24 に threads.net から threads.com へ移行。threads.net は 301 転送。 https://about.fb.com/news/2025/04/new-features-threads-web/ |
| Instagram | `https://www.instagram.com/<username>/` | 27+ | 公式アプリが宣言 | アプリ内にもプロフィール QR 機能あり |
| Facebook | `https://www.facebook.com/<username>` | 25+ | 公式アプリが宣言 | ユーザー名未設定なら `profile.php?id=` |
| LinkedIn | `https://www.linkedin.com/in/<custom>` | 28+ | 公式アプリが `/in/*` | 国別サブドメインが付くことがある |
| GitHub | `https://github.com/<username>` | 19+(最大 58) | 公式アプリが宣言 | ユーザー名は英数字とハイフン、最大 39 文字。大文字化しても同じページ |
| YouTube | `https://www.youtube.com/@<handle>` | 24+ | 公式アプリが宣言 | 日本語ハンドルはパーセントエンコードで長くなり他アプリで扱えないことがある、と公式が注意。`/channel/UC…` 形式が安全。 https://support.google.com/youtube/answer/6180214 |
| note | `https://note.com/<id>` | 17+ | AASA は記事とマガジンのみ、プロフィールはブラウザ | 大文字化は汎用ページになる |
| Zenn | `https://zenn.dev/<username>` | 17+ | AASA なし | 大文字化は 404 |
| Qiita | `https://qiita.com/<user_id>` | 18+ | AASA なし | 大文字化しても同じページ |
| Discord | プロフィール `https://discord.com/users/<数値 ID>`、招待 `https://discord.gg/<code>` | 約 43 / 19+ | 公式アプリが `/users/*` `/invite/*`、discord.gg は全パス | 招待は既定 7 日で失効。固定掲示なら無期限招待かバニティ URL |
| LINE 公式アカウント | `https://line.me/R/ti/p/%40<id>` | 26+ | LINE アプリが `/R/*` | `@` は `%40`。`lin.ee/xxxx` は line.me/R/ への 301 転送。 https://developers.line.biz/ja/docs/messaging-api/sharing-bot/ |
| LINE 個人 | 「マイ QR コード」の `line.me/ti/p/…` | — | `/ti/p/`(R なし)は AASA 対象外 | QR を更新すると URL が変わり旧 URL は無効。固定掲示に向かない |
| Telegram | `https://t.me/<username>` | 13+ | 公式アプリが全パス | `?profile` でプロフィールを開く。英数字モードに最も向く。 https://core.telegram.org/api/links |
| mixi2 | `https://mixi.social/@<id>` | 21+(最大 37) | 公式アプリが全パス | ID は半角英数 4〜16 文字、登録後変更不可(第三者情報) |
| pixiv | `https://www.pixiv.net/users/<数値 ID>` | 約 35 | AASA なし | `pixiv.me/<id>` は数値 ID へ 301。数値 ID が恒久 |
| Twitch | `https://www.twitch.tv/<username>` | 22+ | 公式アプリが宣言 | ユーザー名変更後の旧 URL 転送は情報源で食い違う |
| TikTok | `https://www.tiktok.com/@<username>` | 24+ | 公式アプリが `/@*` | |
| タイッツー | `https://taittsuu.com/users/<username>` | 27+ | 公式アプリが全パス | 非公式 Wiki が出典。大文字化は 404 |
| 技術書典 | `https://techbookfest.org/organization/<数値 ID>` | 約 46 | AASA は `/applink/*` のみ、ブラウザ | 同じ組織 ID が開催回をまたいで使われる |
| BOOTH | `https://<sub>.booth.pm/` | 18+ | AASA なし | サブドメイン変更可否は未確認 |
| 個人サイト | `https://<domain>/` | — | — | スキームとホストは大文字小文字を区別しない(RFC 3986) |
| 連絡先 | `mailto:` か MECARD | — | iOS 11+ の標準カメラは vCard/MECARD に対応 | Android 標準カメラでは項目が落ちる報告あり |

## QR 容量表(出典: https://www.thonky.com/qr-code-tutorial/character-capacities 、モジュール数は DENSO WAVE https://www.qrcode.com/en/about/version.html )

| Ver | モジュール | L 英数字 | L バイト | M 英数字 | M バイト | Q 英数字 | Q バイト |
|---|---|---|---|---|---|---|---|
| 1 | 21 | 25 | 17 | 20 | 14 | 16 | 11 |
| 2 | 25 | 47 | 32 | 38 | 26 | 29 | 20 |
| 3 | 29 | 77 | 53 | 61 | 42 | 47 | 32 |
| 4 | 33 | 114 | 78 | 90 | 62 | 67 | 46 |
| 5 | 37 | 154 | 106 | 122 | 84 | 87 | 60 |
| 6 | 41 | 195 | 134 | 154 | 106 | 108 | 74 |
| 7 | 45 | 224 | 154 | 178 | 122 | 125 | 86 |
| 8 | 49 | 279 | 192 | 221 | 152 | 157 | 108 |

バイトモード・ECC M での目安: X / GitHub / Telegram など 30 文字前後は V3、Bluesky の DID 形式(57)は V4、`nostr:npub`(69)と njump(80)は V5。

## 英数字モード(大文字のみ)を使える条件

- 使える文字は 0-9、A-Z、スペース、`$ % * + - . / :` の 45 種だけ。小文字、`@`、`_`、`?`、`=`、`&`、`#` は入らない。 https://www.thonky.com/qr-code-tutorial/alphanumeric-table
- スキームとホストは大文字にしてよい(RFC 3986)。パスはサーバー次第。
- 実測で大文字パスでも同じページになった: X、GitHub、Telegram、Qiita、YouTube、Mastodon、Threads。404 か汎用ページになった: Bluesky、Zenn、note、タイッツー、pixiv.me、技術書典。
- `@` や `_` を含む URL は 1 モードでは入らない(セグメント混在は実装依存)。
- 結論: この機器では画面が十分大きい(V5 でも 1 モジュール 8px)ので、英数字モードにこだわらずバイトモードで小文字 URL をそのまま入れる。

## この構成での推奨

- 既定は「公式ドメインの https URL をそのままバイトモードで」。
- Nostr だけは用途で選ぶ: Nostr ユーザー向けなら `nostr:npub…`、一般向けなら Web ゲートウェイ(本人のサイトは nostx.io を使用)。
- Bluesky は独自ドメインハンドル(shino3.net / 428lab.net)を使っているので、ハンドル形式のままでよい。
- LINE 個人の QR は URL が変わるので載せない。

## 未確認

実機カメラでの挙動全般、`nostr:` を標準カメラで読んだときの動作、Primal の QR 形式、`twitter://` と `line://` の現状、Bluesky のハンドル変更後の転送、Discord 招待の現行仕様、vCard/MECARD の正式仕様との照合、QR 容量表の DENSO WAVE 一次資料との照合。
