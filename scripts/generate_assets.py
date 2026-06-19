#!/usr/bin/env python3
"""Generate Pebble bitmap assets for Merlin UI."""

from __future__ import annotations

import os
from PIL import Image, ImageDraw

ROOT = os.path.join(os.path.dirname(__file__), "..", "app", "resources")


def save_png(img: Image.Image, path: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.save(path, "PNG")
    print(f"Wrote {path}")


def draw_line(draw: ImageDraw.ImageDraw, p1, p2, width=1):
    draw.line([p1, p2], fill=0, width=width)


def draw_poly_outline(draw: ImageDraw.ImageDraw, points, width=1):
    for i in range(len(points)):
        draw_line(draw, points[i], points[(i + 1) % len(points)], width)


def fill_poly(draw: ImageDraw.ImageDraw, points, fill=1):
    draw.polygon(points, fill=fill)


def draw_merlin_head(draw: ImageDraw.ImageDraw, ox=0, oy=0, scale=1):
    s = scale

    def p(x, y):
        return (ox + x * s, oy + y * s)

    hat = [p(8, 18), p(38, 16), p(36, 8), p(22, 4), p(8, 18)]
    face = [p(10, 22), p(34, 22), p(32, 30), p(12, 30)]
    mustache_l = [p(12, 30), p(20, 32), p(18, 34)]
    mustache_r = [p(34, 30), p(26, 32), p(28, 34)]
    beard = [p(14, 32), p(30, 32), p(32, 42), p(24, 62), p(16, 42)]

    for poly in (hat, face, mustache_l, mustache_r, beard):
        fill_poly(draw, poly, fill=1)

    draw_poly_outline(draw, hat, width=2)
    draw_poly_outline(draw, face, width=2)
    draw_poly_outline(draw, mustache_l, width=1)
    draw_poly_outline(draw, mustache_r, width=1)
    draw_poly_outline(draw, beard, width=2)

    draw_line(draw, p(22, 4), p(28, 0), width=1)
    draw_line(draw, p(28, 0), p(32, 6), width=1)
    draw_line(draw, p(24, 32), p(24, 58), width=1)

    for cx in (16, 28):
        draw.ellipse([p(cx - 4, 20), p(cx + 4, 28)], outline=0, width=1)


def draw_zzz(draw: ImageDraw.ImageDraw, ox, oy):
    draw.text((ox, oy), "Z", fill=0)
    draw.text((ox + 8, oy - 4), "Z", fill=0)
    draw.text((ox + 18, oy - 8), "Z", fill=0)


def make_merlin_head() -> None:
    img = Image.new("1", (56, 64), 0)
    draw = ImageDraw.Draw(img)
    draw_merlin_head(draw)
    save_png(img, os.path.join(ROOT, "images", "merlin_head.png"))


def make_merlin_sleep() -> None:
    img = Image.new("1", (56, 64), 0)
    draw = ImageDraw.Draw(img)
    draw_merlin_head(draw, oy=4)
    draw_zzz(draw, 18, 2)
    save_png(img, os.path.join(ROOT, "images", "merlin_sleep.png"))


def make_icon_help() -> None:
    img = Image.new("1", (28, 28), 0)
    draw = ImageDraw.Draw(img)
    draw.ellipse([4, 4, 23, 23], outline=1, width=2)
    draw.text((10, 6), "?", fill=1)
    save_png(img, os.path.join(ROOT, "icons", "help.png"))


def make_icon_mic() -> None:
    img = Image.new("1", (28, 28), 0)
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle([10, 4, 17, 14], radius=3, outline=1, width=2)
    draw.arc([6, 10, 21, 22], 0, 180, fill=1, width=2)
    draw.line([(13, 22), (13, 25)], fill=1, width=2)
    draw.line([(9, 25), (17, 25)], fill=1, width=2)
    save_png(img, os.path.join(ROOT, "icons", "mic.png"))


def make_icon_more() -> None:
    img = Image.new("1", (28, 28), 0)
    draw = ImageDraw.Draw(img)
    for x in (8, 13, 18):
        draw.ellipse([x, 12, x + 5, 17], fill=1)
    save_png(img, os.path.join(ROOT, "icons", "more.png"))


def make_icon_snooze() -> None:
    img = Image.new("1", (28, 28), 0)
    draw = ImageDraw.Draw(img)
    draw.text((4, 8), "Z", fill=1)
    draw.text((10, 4), "Z", fill=1)
    draw.text((16, 0), "Z", fill=1)
    save_png(img, os.path.join(ROOT, "icons", "snooze.png"))


def make_icon_dismiss() -> None:
    img = Image.new("1", (28, 28), 0)
    draw = ImageDraw.Draw(img)
    draw.line([(6, 6), (21, 21)], fill=1, width=3)
    draw.line([(21, 6), (6, 21)], fill=1, width=3)
    save_png(img, os.path.join(ROOT, "icons", "dismiss.png"))


def main() -> None:
    make_merlin_head()
    make_merlin_sleep()
    make_icon_help()
    make_icon_mic()
    make_icon_more()
    make_icon_snooze()
    make_icon_dismiss()


if __name__ == "__main__":
    main()
