"""M5Stack StopWatch 用 UI モックアップ生成スクリプト。

使い方: .venv/bin/python docs/design/render_mockups.py
出力:   docs/design/{a,b,c}/*.png, docs/design/{a,b,c}/sheet.png, docs/design/compare.png

座標はすべて実機の 468x468 座標(中心 234,234、可視半径 233)で書く。
内部では 2 倍で描いて縮小し、輪郭のギザギザを抑える。
フォントは macOS のヒラギノで代用している(実機は lgfxJapanGothic_*)。
文字サイズの数字は lgfxJapanGothic の番号にそろえてある。
"""

from __future__ import annotations

import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs" / "design"
AVATAR = {
    "shino3": ROOT / "data" / "avatars" / "shino3.jpg",
    "428lab": ROOT / "data" / "avatars" / "428lab.jpg",
}

S = 2          # 描画倍率
W = 468        # 画面一辺
C = 234        # 中心
R = 233        # 可視半径

FONT_FILES = {
    "W3": "/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc",
    "W6": "/System/Library/Fonts/ヒラギノ角ゴシック W6.ttc",
}
_font_cache: dict[tuple[str, int], ImageFont.FreeTypeFont] = {}

# ---- 色 ----
BLACK = (0, 0, 0)
WHITE = (240, 240, 240)
SUB = (140, 140, 150)
DIM = (60, 60, 68)
TRACK = (38, 38, 44)
PANEL = (40, 40, 50)
KEYA = (250, 190, 30)   # 本体 KEYA の黄
KEYB = (60, 130, 255)   # 本体 KEYB の青

# リンク種別: 表示名、頭文字、円の色、文字色、ハンドル
LINKS_SHINO3 = [
    ("x", "X", "X", (235, 235, 235), (0, 0, 0), "@SHINOHARATTT", "x.com/SHINOHARATTT"),
    ("bluesky", "Bluesky", "Bs", (17, 133, 254), WHITE, "@shino3.net", "bsky.app/profile/shino3.net"),
    ("github", "GitHub", "GH", (110, 118, 129), WHITE, "ShinoharaTa", "github.com/ShinoharaTa"),
    ("nostr", "Nostr", "N", (142, 68, 239), WHITE, "npub1l60d…ejxe5", "nostx.io/npub1l60d…"),
    ("instagram", "Instagram", "IG", (225, 48, 108), WHITE, "@shinoharattt", "instagram.com/shinoharattt"),
    ("threads", "Threads", "@", (200, 200, 200), (0, 0, 0), "@shinoharattt", "threads.com/@shinoharattt"),
    ("web", "Web", "", (40, 170, 120), WHITE, "shino3.net", "shino3.net"),
]
SEL_LINK = 1  # モックで選択中のリンク(Bluesky)

APPS = [
    ("Badge", (64, 140, 255)),
    ("Clock", (255, 170, 60)),
    ("Stopwatch", (40, 200, 150)),
    ("Settings", (150, 150, 170)),
]

SETTINGS = [
    ("輝度", "120"),
    ("減光まで", "20 秒"),
    ("消灯まで", "30 秒"),
    ("イベントモード", "OFF"),
    ("ピクセルシフト", "ON"),
    ("電池", "82%"),
    ("バージョン", "0.2.0"),
]


def font(size: int, weight: str = "W6") -> ImageFont.FreeTypeFont:
    key = (weight, size)
    if key not in _font_cache:
        _font_cache[key] = ImageFont.truetype(FONT_FILES[weight], size * S, index=0)
    return _font_cache[key]


def scale(c, k: float):
    return tuple(int(v * k) for v in c)


def polar(r: float, deg: float) -> tuple[float, float]:
    """時計の角度(0=12 時、時計回り)で極座標を返す。"""
    a = math.radians(deg)
    return C + r * math.sin(a), C - r * math.cos(a)


class Canvas:
    def __init__(self, bg=BLACK):
        self.im = Image.new("RGB", (W * S, W * S), bg)
        self.d = ImageDraw.Draw(self.im)

    def redraw(self):
        self.d = ImageDraw.Draw(self.im)

    @staticmethod
    def _b(x0, y0, x1, y1):
        return [x0 * S, y0 * S, x1 * S, y1 * S]

    def circle(self, cx, cy, r, fill=None, outline=None, width=1):
        self.d.ellipse(self._b(cx - r, cy - r, cx + r, cy + r), fill=fill,
                       outline=outline, width=int(width * S))

    def ellipse(self, cx, cy, rx, ry, outline, width):
        self.d.ellipse(self._b(cx - rx, cy - ry, cx + rx, cy + ry), outline=outline, width=int(width * S))

    def arc(self, r, a0, a1, color, width, cx=C, cy=C):
        """中心からの半径 r(線の中心)に、時計角度 a0→a1 の円弧を描く。"""
        ro = r + width / 2
        self.d.arc(self._b(cx - ro, cy - ro, cx + ro, cy + ro), a0 - 90, a1 - 90,
                   fill=color, width=int(width * S))

    def line(self, pts, color, width):
        self.d.line([(x * S, y * S) for x, y in pts], fill=color, width=int(width * S), joint="curve")
        for x, y in (pts[0], pts[-1]):  # 端を丸める
            self.circle(x, y, width / 2, fill=color)

    def rrect(self, x0, y0, x1, y1, rad, fill=None, outline=None, width=1):
        self.d.rounded_rectangle(self._b(x0, y0, x1, y1), radius=rad * S, fill=fill,
                                 outline=outline, width=int(width * S))

    def polygon(self, pts, fill):
        self.d.polygon([(x * S, y * S) for x, y in pts], fill=fill)

    def text(self, x, y, s, size, color=WHITE, weight="W6", anchor="mm"):
        self.d.text((x * S, y * S), s, font=font(size, weight), fill=color, anchor=anchor)

    def text_w(self, s, size, weight="W6") -> float:
        return self.d.textlength(s, font=font(size, weight)) / S

    def paste(self, img: Image.Image, cx, cy, size, mask=None):
        im = img.resize((int(size * S), int(size * S)), Image.LANCZOS)
        self.im.paste(im, (int((cx - size / 2) * S), int((cy - size / 2) * S)), mask)

    def overlay(self, rgba: Image.Image):
        base = self.im.convert("RGBA")
        base.alpha_composite(rgba)
        self.im = base.convert("RGB")
        self.redraw()

    def dim(self, k: float):
        self.im = ImageEnhance.Brightness(self.im).enhance(k)
        self.redraw()

    def finish(self) -> Image.Image:
        mask = Image.new("L", self.im.size, 0)
        ImageDraw.Draw(mask).ellipse(self._b(C - R, C - R, C + R, C + R), fill=255)
        out = Image.new("RGB", self.im.size, BLACK)
        out.paste(self.im, (0, 0), mask)
        self.im = out
        self.redraw()
        self.circle(C, C, R + 1, outline=(80, 80, 88), width=2)  # 本体の縁
        return self.im.resize((W, W), Image.LANCZOS)


# ---- 部品 ----

def avatar(name: str) -> Image.Image:
    return Image.open(AVATAR[name]).convert("RGB")


def chevron(cv: Canvas, x, y, direction: int, color=WHITE, arm=22, width=5):
    """direction=-1 で「<」、+1 で「>」。"""
    cv.line([(x - direction * arm * 0.55, y - arm), (x + direction * arm * 0.55, y),
             (x - direction * arm * 0.55, y + arm)], color, width)


def dots(cv: Canvas, y, n, sel, gap=20, r_on=6, r_off=3.5, on=WHITE, off=DIM):
    x0 = C - gap * (n - 1) / 2
    for i in range(n):
        if i == sel:
            cv.circle(x0 + gap * i, y, r_on, fill=on)
        else:
            cv.circle(x0 + gap * i, y, r_off, fill=off)


def switch(cv: Canvas, x, y, on: bool, accent):
    cv.rrect(x - 22, y - 12, x + 22, y + 12, 12, fill=accent if on else (70, 70, 80))
    cv.circle(x + (10 if on else -10), y, 9, fill=WHITE)


def battery_icon(cv: Canvas, x, y, pct, color=WHITE, w=34, h=16):
    cv.rrect(x - w / 2, y - h / 2, x + w / 2, y + h / 2, 3, outline=color, width=2)
    cv.rrect(x + w / 2 + 1, y - 4, x + w / 2 + 4, y + 4, 1, fill=color)
    fw = (w - 6) * pct / 100
    cv.rrect(x - w / 2 + 3, y - h / 2 + 3, x - w / 2 + 3 + fw, y + h / 2 - 3, 1, fill=color)


def globe(cv: Canvas, cx, cy, r, color, w):
    cv.circle(cx, cy, r, outline=color, width=w)
    cv.ellipse(cx, cy, r * 0.42, r, color, w)
    cv.line([(cx - r, cy), (cx + r, cy)], color, w)
    for dy in (-0.5, 0.5):
        hw = r * math.sqrt(1 - dy * dy)
        cv.line([(cx - hw, cy + dy * r), (cx + hw, cy + dy * r)], color, w * 0.8)


def link_icon(cv: Canvas, cx, cy, r, link, faded=False):
    kind, _name, initial, col, fg, *_ = link
    if faded:
        cv.circle(cx, cy, r, fill=scale(col, 0.28))
        fg = scale(col, 0.9) if kind not in ("x", "threads") else (200, 200, 200)
    else:
        cv.circle(cx, cy, r, fill=col)
    if kind == "web":
        globe(cv, cx, cy, r * 0.58, fg, max(2, r * 0.07))
    else:
        size = int(r * (1.0 if len(initial) == 1 else 0.78))
        cv.text(cx, cy + r * 0.03, initial, size, fg, "W6")


def app_glyph(cv: Canvas, kind: str, cx, cy, s, color, bg):
    """s は外接円の半径程度。M5GFX のプリミティブで描ける形だけを使う。"""
    w = max(3, s * 0.11)
    if kind == "Badge":
        cv.circle(cx, cy - s * 0.25, s * 0.26, fill=color)
        cv.d.chord(cv._b(cx - s * 0.5, cy + s * 0.08, cx + s * 0.5, cy + s * 1.0), 180, 360, fill=color)
    elif kind == "Clock":
        cv.circle(cx, cy, s * 0.58, outline=color, width=w)
        cv.line([(cx, cy), (cx, cy - s * 0.36)], color, w)
        cv.line([(cx, cy), (cx + s * 0.26, cy + s * 0.08)], color, w)
    elif kind == "Stopwatch":
        cy2 = cy + s * 0.08
        cv.circle(cx, cy2, s * 0.52, outline=color, width=w)
        cv.rrect(cx - s * 0.13, cy2 - s * 0.80, cx + s * 0.13, cy2 - s * 0.66, 2, fill=color)
        cv.line([(cx, cy2 - s * 0.66), (cx, cy2 - s * 0.54)], color, w)
        ex, ey = cx + s * 0.30 * math.sin(math.radians(40)), cy2 - s * 0.30 * math.cos(math.radians(40))
        cv.line([(cx, cy2), (ex, ey)], color, w)
    elif kind == "Settings":
        for i in range(8):
            a = math.radians(i * 45)
            pts = []
            for dx, dy in ((-0.13, 0.30), (0.13, 0.30), (0.10, 0.62), (-0.10, 0.62)):
                x, y = dx * s, -dy * s
                pts.append((cx + x * math.cos(a) - y * math.sin(a), cy + x * math.sin(a) + y * math.cos(a)))
            cv.polygon(pts, color)
        cv.circle(cx, cy, s * 0.42, fill=color)
        cv.circle(cx, cy, s * 0.18, fill=bg)


def fake_qr(version: int, seed: int = 7) -> list[list[int]]:
    n = 17 + 4 * version
    rnd = random.Random(seed)
    m = [[1 if rnd.random() < 0.5 else 0 for _ in range(n)] for _ in range(n)]

    def finder(ox, oy):
        for y in range(-1, 8):
            for x in range(-1, 8):
                xx, yy = ox + x, oy + y
                if 0 <= xx < n and 0 <= yy < n:
                    on = 0 <= x <= 6 and 0 <= y <= 6 and (x in (0, 6) or y in (0, 6) or (2 <= x <= 4 and 2 <= y <= 4))
                    m[yy][xx] = 1 if on else 0

    finder(0, 0)
    finder(n - 7, 0)
    finder(0, n - 7)
    for i in range(8, n - 8):
        m[6][i] = m[i][6] = 1 if i % 2 == 0 else 0
    if version >= 2:
        a = n - 7
        for y in range(-2, 3):
            for x in range(-2, 3):
                m[a + y][a + x] = 1 if max(abs(x), abs(y)) != 1 else 0
    return m


def qr_block(cv: Canvas, version=3, module=8, cy=C):
    m = fake_qr(version)
    n = len(m)
    size = n * module
    x0, y0 = C - size / 2, cy - size / 2
    for y in range(n):
        for x in range(n):
            if m[y][x]:
                cv.d.rectangle(cv._b(x0 + x * module, y0 + y * module,
                                     x0 + (x + 1) * module, y0 + (y + 1) * module), fill=(0, 0, 0))
    return size


def hint(cv: Canvas, a_label, b_label, label_r=200, alpha=1.0, size=16):
    """本体の KEYA(10:30 方向)と KEYB(1:30 方向)の内側に役割を出す。"""
    ca, cb = scale(KEYA, alpha), scale(KEYB, alpha)
    cv.arc(224, 306, 324, ca, 6)
    cv.arc(224, 36, 54, cb, 6)
    if a_label:
        x, y = polar(label_r, 296)
        cv.text(x, y, a_label, size, ca)
    if b_label:
        x, y = polar(label_r, 64)
        cv.text(x, y, b_label, size, cb)


def gradient(y0, y1, a0, a1, top=False) -> Image.Image:
    g = Image.new("RGBA", (W * S, W * S), (0, 0, 0, 0))
    px = g.load()
    for y in range(W * S):
        yy = y / S
        if top:
            t = 1 - (yy - y0) / (y1 - y0)
        else:
            t = (yy - y0) / (y1 - y0)
        t = min(1, max(0, t))
        a = int(a0 + (a1 - a0) * t)
        if a:
            for x in range(W * S):
                px[x, y] = (0, 0, 0, a)
    return g


_grad_cache: dict = {}


def grad(*args, **kw):
    key = (args, tuple(sorted(kw.items())))
    if key not in _grad_cache:
        _grad_cache[key] = gradient(*args, **kw)
    return _grad_cache[key]


# ============================================================
# 案 A: 標準ファーム準拠(1 ページ 1 項目の横カルーセル)
# ============================================================
A_ACC = (20, 224, 176)


def a_launcher():
    cv = Canvas()
    sel = 0
    name, col = APPS[sel]
    cv.text(C, 50, "10:42", 24, WHITE)
    cv.circle(C, 210, 96, fill=scale(col, 0.32))
    app_glyph(cv, name, C, 210, 96, col, scale(col, 0.32))
    chevron(cv, 62, 210, -1)
    chevron(cv, 406, 210, +1)
    cv.text(C, 345, name, 32, WHITE)
    dots(cv, 392, len(APPS), sel)
    return cv.finish()


def a_badge(cv=None):
    cv = cv or Canvas()
    cv.paste(avatar("shino3"), C, 200, 320)
    cv.text(C, 395, "shino3", 32, WHITE)
    dots(cv, 434, 2, 0, gap=18, r_on=5, r_off=3.5)
    return cv


def a_links():
    cv = Canvas()
    link = LINKS_SHINO3[SEL_LINK]
    cv.text(C, 52, "shino3", 20, SUB, "W3")
    link_icon(cv, C, 200, 88, link)
    chevron(cv, 62, 200, -1)
    chevron(cv, 406, 200, +1)
    cv.text(C, 328, link[1], 32, WHITE)
    cv.text(C, 366, link[5], 20, SUB, "W3")
    dots(cv, 406, len(LINKS_SHINO3), SEL_LINK)
    return cv.finish()


def a_qr():
    cv = Canvas(bg=(255, 255, 255))
    qr_block(cv, 3, 8)
    link = LINKS_SHINO3[SEL_LINK]
    cv.text(C, 60, f"{link[1]}  {SEL_LINK + 1}/{len(LINKS_SHINO3)}", 24, (30, 30, 30))
    cv.text(C, 406, link[6], 16, (60, 60, 60), "W3")
    return cv.finish()


def a_settings():
    cv = Canvas()
    sel = 3
    cv.text(C, 52, "Settings", 20, SUB, "W3")
    for i in range(sel - 2, sel + 3):
        label, val = SETTINGS[i]
        y = C + (i - sel) * 62
        if i == sel:
            cv.rrect(70, y - 27, 398, y + 27, 27, fill=PANEL)
            cv.text(98, y, label, 24, WHITE, anchor="lm")
            if val in ("ON", "OFF"):
                switch(cv, 360, y, val == "ON", A_ACC)
            else:
                cv.text(372, y, val, 24, A_ACC, anchor="rm")
        else:
            cv.text(98, y, label, 20, SUB, "W3", anchor="lm")
            if val in ("ON", "OFF"):
                switch(cv, 360, y, val == "ON", scale(A_ACC, 0.6))
            else:
                cv.text(372, y, val, 20, SUB, "W3", anchor="rm")
    # 右端のスクロール位置(円弧)
    cv.arc(220, 60, 120, TRACK, 5)
    seg = 60 / len(SETTINGS)
    cv.arc(220, 60 + seg * sel, 60 + seg * (sel + 1), WHITE, 5)
    return cv.finish()


def a_status():
    cv = a_badge(Canvas())
    cv.dim(0.35)
    cv.rrect(114, 26, 354, 150, 30, fill=PANEL)
    cv.text(C, 58, "10:42", 24, WHITE)
    battery_icon(cv, 170, 106, 82, WHITE, w=44, h=22)
    cv.text(208, 106, "82%", 32, WHITE, anchor="lm")
    return cv.finish()


# ============================================================
# 案 B: リング・ダイヤル(円周に項目、中央に詳細)
# ============================================================
B_ACC = KEYA


def b_ring(cv: Canvas, items, sel, ring_r, r_item, r_sel, draw_item):
    n = len(items)
    cv.circle(C, C, ring_r, outline=TRACK, width=2)
    for k in range(n):
        i = (sel + k) % n
        x, y = polar(ring_r, 360 / n * k)
        if k == 0:
            cv.circle(x, y, r_sel + 6, outline=B_ACC, width=3)
            cv.circle(x, y, r_sel + 2, fill=BLACK)
            draw_item(x, y, r_sel, items[i], False)
        else:
            cv.circle(x, y, r_item + 3, fill=BLACK)
            draw_item(x, y, r_item, items[i], True)


def b_launcher():
    cv = Canvas()
    sel = 0

    def draw_app(x, y, r, app, faded):
        name, col = app
        bg = scale(col, 0.22 if faded else 0.32)
        cv.circle(x, y, r, fill=bg)
        app_glyph(cv, name, x, y, r * 0.8, scale(col, 0.8) if faded else WHITE, bg)

    b_ring(cv, APPS, sel, 170, 26, 32, draw_app)
    name, col = APPS[sel]
    cv.circle(C, 206, 62, fill=scale(col, 0.32))
    app_glyph(cv, name, C, 206, 62, col, scale(col, 0.32))
    cv.text(C, 300, name, 28, WHITE)
    cv.text(C, 334, "10:42", 20, SUB, "W3")
    hint(cv, "起動", "戻る")
    return cv.finish()


def b_badge(cv=None, profile_sel=0):
    cv = cv or Canvas()
    cv.paste(avatar("shino3"), C, C, 280)
    cv.circle(C, C, 152, outline=TRACK, width=2)
    # プロファイル位置: 12 時の短い円弧
    for i in range(2):
        a0 = -16 + i * 18
        cv.arc(204, a0, a0 + 14, WHITE if i == profile_sel else DIM, 5)
    cv.text(C, 408, "shino3", 28, WHITE)
    hint(cv, "リンク", "アプリ", alpha=0.55)
    return cv


def b_links():
    cv = Canvas()

    def draw_link(x, y, r, link, faded):
        link_icon(cv, x, y, r, link, faded)

    b_ring(cv, LINKS_SHINO3, SEL_LINK, 168, 24, 30, draw_link)
    link = LINKS_SHINO3[SEL_LINK]
    link_icon(cv, C, 222, 60, link)
    cv.text(C, 306, link[1], 28, WHITE)
    cv.text(C, 338, link[5], 16, SUB, "W3")
    hint(cv, "QR", "戻る")
    return cv.finish()


def b_qr():
    cv = Canvas(bg=(255, 255, 255))
    qr_block(cv, 3, 8)
    link = LINKS_SHINO3[SEL_LINK]
    n = len(LINKS_SHINO3)
    for i in range(n):  # 位置: 上端の円弧に沿った点
        x, y = polar(214, (i - (n - 1) / 2) * 6)
        cv.circle(x, y, 5 if i == SEL_LINK else 3.5, fill=(40, 40, 40) if i == SEL_LINK else (190, 190, 190))
    w = cv.text_w(link[1], 20) + 36
    cv.rrect(C - w / 2, 46, C + w / 2, 78, 16, fill=link[3])
    cv.text(C, 62, link[1], 20, link[4])
    cv.text(C, 404, link[6], 16, (60, 60, 60), "W3")
    return cv.finish()


def b_settings():
    cv = Canvas()
    sel = 0
    label, val = SETTINGS[sel]
    cv.arc(206, -135, 135, TRACK, 16)
    end = -135 + 270 * 120 / 255
    cv.arc(206, -135, end, B_ACC, 16)
    kx, ky = polar(214, end)
    cv.circle(kx, ky, 13, fill=WHITE)
    cv.text(C, 122, f"Settings  {sel + 1}/{len(SETTINGS)}", 16, SUB, "W3")
    cv.text(C, 178, label, 28, WHITE)
    cv.text(C, 248, val, 80, WHITE)
    cv.text(C, 310, "/ 255", 20, SUB, "W3")
    dots(cv, 352, len(SETTINGS), sel, gap=18, r_on=5, r_off=3)
    cv.text(C, 418, "タップで編集", 16, SUB, "W3")
    hint(cv, "", "", alpha=0.55)
    return cv.finish()


def b_status():
    cv = b_badge(Canvas())
    cv.arc(158, 0, 360 * 0.82, (60, 200, 120), 6)
    cv.text(C, 62, "82%", 20, (60, 200, 120))
    return cv.finish()


# ============================================================
# 案 C: 全面アバター + 縦ホイール
# ============================================================
C_ACC = (255, 96, 128)


def c_badge(cv=None):
    cv = cv or Canvas()
    cv.paste(avatar("shino3"), C, C, 480)
    cv.overlay(grad(290, 468, 0, 235))
    cv.text(C, 390, "shino3", 36, WHITE)
    dots(cv, 428, 2, 0, gap=18, r_on=5, r_off=3.5, off=(110, 110, 110))
    return cv


def c_wheel(cv: Canvas, rows, sel, draw_row, spacing=(0, 90, 150)):
    for i, row in enumerate(rows):
        k = i - sel
        if abs(k) > 2:
            continue
        y = C + (spacing[abs(k)] if k >= 0 else -spacing[abs(k)])
        draw_row(y, row, abs(k))


def c_launcher():
    cv = Canvas()
    sel = 1
    cv.text(C, 54, "10:42", 20, SUB, "W3")

    def draw(y, app, level):
        name, col = app
        if level == 0:
            cv.rrect(52, y - 40, 416, y + 40, 40, fill=PANEL)
            cv.circle(112, y, 28, fill=scale(col, 0.32))
            app_glyph(cv, name, 112, y, 28, col, scale(col, 0.32))
            cv.text(160, y, name, 40, WHITE, anchor="lm")
        else:
            k = 0.75 if level == 1 else 0.45
            r = 20 if level == 1 else 15
            cv.circle(136, y, r, fill=scale(col, 0.25 * k))
            app_glyph(cv, name, 136, y, r, scale(col, k), scale(col, 0.25 * k))
            cv.text(172, y, name, 24 if level == 1 else 20, scale(WHITE, k), "W3", anchor="lm")

    c_wheel(cv, APPS, sel, draw)
    cv.arc(222, 70, 110, TRACK, 5)
    seg = 40 / len(APPS)
    cv.arc(222, 70 + seg * sel, 70 + seg * (sel + 1), WHITE, 5)
    return cv.finish()


def c_links():
    cv = Canvas()
    bg = avatar("shino3").resize((520, 520), Image.LANCZOS).filter(ImageFilter.GaussianBlur(14))
    bg = ImageEnhance.Brightness(bg).enhance(0.22)
    cv.paste(bg, C, C, 520)
    cv.text(C, 66, "shino3", 20, SUB, "W3")

    def draw(y, link, level):
        if level == 0:
            cv.rrect(52, y - 42, 416, y + 42, 42, fill=(16, 16, 20), outline=link[3], width=2)
            link_icon(cv, 110, y, 28, link)
            cv.text(154, y - 10, link[1], 32, WHITE, anchor="lm")
            cv.text(156, y + 22, link[5], 16, SUB, "W3", anchor="lm")
        else:
            k = 0.8 if level == 1 else 0.5
            r = 18 if level == 1 else 14
            link_icon(cv, 136, y, r, link, faded=True)
            cv.text(168, y, link[1], 24 if level == 1 else 20, scale(WHITE, k), "W3", anchor="lm")

    c_wheel(cv, LINKS_SHINO3, SEL_LINK, draw, spacing=(0, 92, 152))
    n = len(LINKS_SHINO3)
    cv.arc(222, 66, 114, TRACK, 5)
    seg = 48 / n
    cv.arc(222, 66 + seg * SEL_LINK, 66 + seg * (SEL_LINK + 1), WHITE, 5)
    return cv.finish()


def c_qr():
    cv = Canvas(bg=(255, 255, 255))
    qr_block(cv, 3, 8)
    av = avatar("shino3")
    m = Image.new("L", (64 * S, 64 * S), 0)
    ImageDraw.Draw(m).ellipse([0, 0, 64 * S - 1, 64 * S - 1], fill=255)
    cv.paste(av, C, 50, 64, mask=m)
    link = LINKS_SHINO3[SEL_LINK]
    cv.text(C, 402, f"{link[1]} · {SEL_LINK + 1}/{len(LINKS_SHINO3)}", 20, (30, 30, 30))
    cv.text(C, 428, link[6], 16, (90, 90, 90), "W3")
    return cv.finish()


def c_settings():
    cv = Canvas()
    sel = 3
    cv.text(C, 46, "Settings", 16, SUB, "W3")

    def draw(y, item, level):
        label, val = item
        if level == 0:
            cv.rrect(52, y - 38, 416, y + 38, 38, fill=PANEL)
            cv.text(88, y, label, 28, WHITE, anchor="lm")
            if val in ("ON", "OFF"):
                switch(cv, 372, y, val == "ON", C_ACC)
            else:
                cv.text(388, y, val, 28, C_ACC, anchor="rm")
        else:
            k = 0.75 if level == 1 else 0.45
            sz = 24 if level == 1 else 20
            cv.text(100, y, label, sz, scale(WHITE, k), "W3", anchor="lm")
            if val in ("ON", "OFF"):
                switch(cv, 360, y, val == "ON", scale(C_ACC, k * 0.7))
            else:
                cv.text(380, y, val, sz, scale(WHITE, k), "W3", anchor="rm")

    c_wheel(cv, SETTINGS, sel, draw, spacing=(0, 80, 136))
    cv.arc(222, 66, 114, TRACK, 5)
    seg = 48 / len(SETTINGS)
    cv.arc(222, 66 + seg * sel, 66 + seg * (sel + 1), WHITE, 5)
    return cv.finish()


def c_status():
    cv = c_badge(Canvas())
    cv.overlay(grad(0, 150, 0, 220, top=True))
    cv.rrect(160, 36, 308, 76, 20, fill=(28, 28, 34))
    battery_icon(cv, 196, 56, 82, WHITE, w=30, h=14)
    cv.text(220, 56, "82%", 20, WHITE, anchor="lm")
    cv.text(C, 104, "10:42", 20, (200, 200, 200), "W3")
    return cv.finish()


# ============================================================
# 書き出し
# ============================================================

SCREENS = [
    ("launcher", "ランチャー"),
    ("badge", "Badge"),
    ("links", "リンクメニュー"),
    ("qr", "QR 全画面"),
    ("settings", "Settings"),
    ("status", "復帰・電池"),
]

PLANS = {
    "a": ("案 A: 標準ファーム準拠カルーセル", {
        "launcher": a_launcher, "badge": lambda: a_badge().finish(), "links": a_links,
        "qr": a_qr, "settings": a_settings, "status": a_status}),
    "b": ("案 B: リング・ダイヤル", {
        "launcher": b_launcher, "badge": lambda: b_badge().finish(), "links": b_links,
        "qr": b_qr, "settings": b_settings, "status": b_status}),
    "c": ("案 C: 全面アバター + 縦ホイール", {
        "launcher": c_launcher, "badge": lambda: c_badge().finish(), "links": c_links,
        "qr": c_qr, "settings": c_settings, "status": c_status}),
}

SHEET_BG = (28, 28, 32)


def label_font(size):
    return ImageFont.truetype(FONT_FILES["W6"], size, index=0)


def sheet(title: str, images: list[tuple[str, Image.Image]]) -> Image.Image:
    gap, top, bottom = 28, 76, 56
    w = gap + len(images) * (W + gap)
    h = top + W + bottom
    out = Image.new("RGB", (w, h), SHEET_BG)
    d = ImageDraw.Draw(out)
    d.text((gap, 24), title, font=label_font(32), fill=WHITE)
    for i, (label, im) in enumerate(images):
        x = gap + i * (W + gap)
        out.paste(im, (x, top))
        d.text((x + W / 2, top + W + 28), label, font=label_font(22), fill=SUB, anchor="mm")
    return out


def main():
    results: dict[str, dict[str, Image.Image]] = {}
    for key, (title, funcs) in PLANS.items():
        d = OUT / key
        d.mkdir(parents=True, exist_ok=True)
        results[key] = {}
        for screen, _label in SCREENS:
            im = funcs[screen]()
            im.save(d / f"{screen}.png")
            results[key][screen] = im
        sheet(title, [(lbl, results[key][s]) for s, lbl in SCREENS]).save(d / "sheet.png")
        print("wrote", d)

    # 比較: 列 = 案、行 = Badge / リンクメニュー
    gap, top, rowlab = 28, 70, 40
    cols = list(PLANS)
    w = gap + len(cols) * (W + gap)
    h = top + 2 * (W + rowlab + gap)
    out = Image.new("RGB", (w, h), SHEET_BG)
    dr = ImageDraw.Draw(out)
    for ci, key in enumerate(cols):
        x = gap + ci * (W + gap)
        dr.text((x + W / 2, 36), PLANS[key][0], font=label_font(26), fill=WHITE, anchor="mm")
        for ri, screen in enumerate(("badge", "links")):
            y = top + ri * (W + rowlab + gap)
            out.paste(results[key][screen], (x, y))
            dr.text((x + W / 2, y + W + 20), dict(SCREENS)[screen], font=label_font(20),
                    fill=SUB, anchor="mm")
    out.save(OUT / "compare.png")
    print("wrote", OUT / "compare.png")


if __name__ == "__main__":
    main()
