#!/usr/bin/env python3
"""config.yaml を本体用の data/ に変換する。

  data/config.json        本体が ArduinoJson で読む設定
  data/avatars/<id>.jpg   円形にトリミングして黒背景に載せた JPEG

使い方:
  .venv/bin/python tools/build_assets.py                       # config.yaml → data/
  .venv/bin/python tools/build_assets.py --config config.example.yaml --out /tmp/out

依存: pyyaml, pillow(tools/requirements.txt)。SVG のアバターは rsvg-convert(brew の librsvg)で PNG 化する。
本体では YAML を解釈しないので、人が触るのは config.yaml だけ。
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import yaml
from PIL import Image, ImageDraw, ImageOps

# バイトモード・ECC M の収容バイト数(V1〜V20)。docs/research-sns.md の容量表と同じ出典
QR_BYTE_CAPACITY_M = [14, 26, 42, 62, 84, 106, 122, 152, 180, 213, 251, 287, 331, 362, 412, 450, 504, 560, 624, 666]
ID_RE = re.compile(r"^[a-z0-9_-]+$")
DEVICE_KEYS = {
    "brightness": 120,
    "qr_brightness": 255,
    "dim_after_sec": 20,
    "off_after_sec": 30,
    "standby_after_min": 5,
    "qr_timeout_sec": 60,
    "wake_on_wrist": True,
    "pixel_shift": True,
    "flip": True,
}


class ConfigError(Exception):
    pass


def estimate_qr_version(url: str) -> int | None:
    n = len(url.encode("utf-8"))
    for version, cap in enumerate(QR_BYTE_CAPACITY_M, start=1):
        if n <= cap:
            return version
    return None


def default_label(url: str) -> str:
    s = url
    for prefix in ("https://", "http://"):
        if s.startswith(prefix):
            s = s[len(prefix):]
            break
    if s.startswith("www."):
        s = s[4:]
    return s.rstrip("/")


def load_source_image(path: Path, size: int) -> Image.Image:
    """PNG/JPEG/SVG を RGBA で開く。SVG は rsvg-convert で size×size に描く。"""
    if path.suffix.lower() == ".svg":
        tool = shutil.which("rsvg-convert")
        if not tool:
            raise ConfigError(f"{path}: SVG を描くには rsvg-convert が必要(brew install librsvg)")
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "avatar.png"
            subprocess.run(
                [tool, "-w", str(size), "-h", str(size), "-f", "png", "-o", str(out), str(path)],
                check=True,
            )
            return Image.open(out).convert("RGBA")
    return Image.open(path).convert("RGBA")


def make_avatar(src: Image.Image, size: int, fit: str) -> Image.Image:
    """円形に切り抜き、黒背景の RGB にする。cover は中央トリミング、contain は余白付きで全体を収める。"""
    if fit == "cover":
        square = ImageOps.fit(src, (size, size), method=Image.LANCZOS, centering=(0.5, 0.5))
    elif fit == "contain":
        inner = int(size * 0.72)  # 円に内接する正方形(0.707)より少し大きめ。角は円で切れる
        scaled = ImageOps.contain(src, (inner, inner), method=Image.LANCZOS)
        square = Image.new("RGBA", (size, size), (0, 0, 0, 0))
        square.paste(scaled, ((size - scaled.width) // 2, (size - scaled.height) // 2), scaled)
    else:
        raise ConfigError(f"avatar_fit は cover か contain: {fit}")

    mask = Image.new("L", (size * 4, size * 4), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, size * 4 - 1, size * 4 - 1), fill=255)
    mask = mask.resize((size, size), Image.LANCZOS)  # 縁をなめらかに

    black = Image.new("RGBA", (size, size), (0, 0, 0, 255))
    composed = Image.alpha_composite(black, square)  # 透明部分を黒に
    out = Image.new("RGB", (size, size), (0, 0, 0))
    out.paste(composed.convert("RGB"), (0, 0), mask)
    return out


def build(config_path: Path, out_dir: Path, size: int) -> int:
    raw = yaml.safe_load(config_path.read_text(encoding="utf-8"))
    if not isinstance(raw, dict):
        raise ConfigError(f"{config_path}: YAML の最上位が辞書ではない")
    base_dir = config_path.parent

    device_in = raw.get("device") or {}
    device = {k: device_in.get(k, v) for k, v in DEVICE_KEYS.items()}
    event_in = device_in.get("event_mode") or {}
    device["event_mode"] = {
        "enabled": bool(event_in.get("enabled", False)),
        "brightness": int(event_in.get("brightness", 40)),
    }
    for key in ("brightness", "qr_brightness"):
        if not 0 <= int(device[key]) <= 255:
            raise ConfigError(f"device.{key} は 0〜255: {device[key]}")
        device[key] = int(device[key])
    for key in ("dim_after_sec", "off_after_sec", "standby_after_min", "qr_timeout_sec"):
        device[key] = int(device[key])
    unknown = set(device_in) - set(DEVICE_KEYS) - {"event_mode"}
    if unknown:
        print(f"警告: device の知らないキーを無視: {sorted(unknown)}", file=sys.stderr)

    profiles_in = raw.get("profiles") or []
    if not profiles_in:
        raise ConfigError("profiles が空")

    avatars_dir = out_dir / "avatars"
    out_dir.mkdir(parents=True, exist_ok=True)
    avatars_dir.mkdir(exist_ok=True)

    profiles = []
    seen_ids: set[str] = set()
    for i, p in enumerate(profiles_in):
        pid = str(p.get("id") or "").strip()
        if not ID_RE.match(pid):
            raise ConfigError(f"profiles[{i}].id は英小文字・数字・_・- のみ: {pid!r}")
        if pid in seen_ids:
            raise ConfigError(f"profiles[{i}].id が重複: {pid}")
        seen_ids.add(pid)
        name = str(p.get("name") or pid)

        avatar_out = ""
        avatar_size = int(p.get("avatar_size", size))
        avatar_src = p.get("avatar")
        if avatar_src:
            src_path = (base_dir / str(avatar_src)).resolve()
            if not src_path.is_file():
                raise ConfigError(f"profiles[{i}].avatar が見つからない: {src_path}")
            fit = str(p.get("avatar_fit", "cover"))
            img = make_avatar(load_source_image(src_path, avatar_size), avatar_size, fit)
            dest = avatars_dir / f"{pid}.jpg"
            img.save(dest, "JPEG", quality=90, optimize=True)
            avatar_out = f"/avatars/{pid}.jpg"
            print(f"{pid}: {src_path.name} → {dest.relative_to(out_dir.parent) if dest.is_relative_to(out_dir.parent) else dest}"
                  f" ({avatar_size}x{avatar_size}, {fit}, {dest.stat().st_size // 1024} KB)")

        links = []
        for j, l in enumerate(p.get("links") or []):
            if not isinstance(l, dict):
                raise ConfigError(f"profiles[{i}].links[{j}] は kind と url を持つ辞書")
            url = str(l.get("url") or "").strip()
            kind = str(l.get("kind") or "web").strip().lower()
            if not url:
                raise ConfigError(f"profiles[{i}].links[{j}] に url が無い")
            if any(c.isspace() for c in url):
                raise ConfigError(f"profiles[{i}].links[{j}].url に空白が入っている: {url!r}")
            label = str(l.get("label") or default_label(url))
            version = estimate_qr_version(url)
            if version is None:
                raise ConfigError(f"profiles[{i}].links[{j}].url が長すぎる({len(url.encode())} バイト)")
            links.append({"kind": kind, "label": label, "url": url})
            print(f"    {kind:10s} {len(url.encode()):3d} バイト → QR V{version:<2d} {label}")

        profiles.append({
            "id": pid,
            "name": name,
            "avatar": avatar_out,
            "avatar_size": avatar_size,
            "links": links,
        })

    config_out = {"version": 1, "device": device, "profiles": profiles}
    (out_dir / "config.json").write_text(
        json.dumps(config_out, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print(f"書き出し: {out_dir / 'config.json'}(プロファイル {len(profiles)} 件)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="config.yaml → data/config.json とアバター画像")
    parser.add_argument("--config", default="config.yaml", help="入力の YAML(既定: config.yaml)")
    parser.add_argument("--out", default="data", help="出力先(既定: data)。pio run -t uploadfs がここを書き込む")
    parser.add_argument("--size", type=int, default=280, help="アバターの一辺 px(既定: 280。profiles[].avatar_size で個別指定可)")
    args = parser.parse_args()
    try:
        return build(Path(args.config), Path(args.out), args.size)
    except ConfigError as e:
        print(f"エラー: {e}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as e:
        print(f"エラー: 外部コマンドが失敗: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
