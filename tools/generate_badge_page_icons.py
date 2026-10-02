#!/usr/bin/env python3
"""Generate original 80px badge page icons and LVGL ARGB8888 data."""
from __future__ import annotations

from math import cos, pi, sin
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "images"
SIZE = 80
SCALE = 4
INK = (162, 255, 105, 255)
DIM = (87, 153, 77, 255)
DARK = (6, 18, 10, 255)


def pt(x, y):
    return (round(x * SCALE), round(y * SCALE))


def box(x0, y0, x1, y1):
    return (*pt(x0, y0), *pt(x1, y1))


def bezier(a, b, c, d, steps=16):
    points = []
    for i in range(steps + 1):
        t = i / steps
        u = 1 - t
        points.append(pt(u**3*a[0] + 3*u*u*t*b[0] + 3*u*t*t*c[0] + t**3*d[0],
                         u**3*a[1] + 3*u*u*t*b[1] + 3*u*t*t*c[1] + t**3*d[1]))
    return points


def dial(draw):
    draw.ellipse(box(4, 4, 75, 75), outline=DIM, width=2*SCALE)
    for x0, y0, x1, y1 in ((37, 1, 43, 1), (37, 78, 43, 78),
                           (1, 37, 1, 43), (78, 37, 78, 43)):
        draw.line([pt(x0, y0), pt(x1, y1)], fill=INK, width=2*SCALE)


def water(draw, image):
    del image
    contour = (bezier((40, 15), (35, 25), (22, 35), (22, 46)) +
               bezier((22, 46), (22, 59), (30, 65), (40, 65))[1:] +
               bezier((40, 65), (50, 65), (58, 59), (58, 46))[1:] +
               bezier((58, 46), (58, 35), (45, 25), (40, 15))[1:])
    draw.polygon(contour, fill=DARK)
    draw.line(contour + [contour[0]], fill=INK, width=3*SCALE, joint='curve')
    draw.line([pt(29, 52), pt(34, 50), pt(40, 52), pt(46, 50), pt(51, 52)],
              fill=DIM, width=2*SCALE, joint='curve')
    draw.line([pt(31, 41), pt(34, 35)], fill=INK, width=2*SCALE)


def system(draw, image):
    del image
    points = []
    for i in range(48):
        angle = 2*pi*i/48 - pi/2
        radius = 29 if i % 6 in (0, 1, 2) else 23
        points.append(pt(40 + radius*cos(angle), 40 + radius*sin(angle)))
    draw.polygon(points, fill=DARK)
    draw.line(points + [points[0]], fill=INK, width=3*SCALE, joint='curve')
    draw.ellipse(box(27, 27, 53, 53), fill=DARK, outline=DIM, width=3*SCALE)
    draw.rounded_rectangle(box(35, 35, 45, 45), radius=2*SCALE, fill=INK)


def create(name, artwork):
    image = Image.new('RGBA', (SIZE*SCALE, SIZE*SCALE))
    draw = ImageDraw.Draw(image)
    dial(draw)
    artwork(draw, image)
    image = image.resize((SIZE, SIZE), Image.Resampling.LANCZOS)
    image.save(OUT / f'badge-icon-{name}.png')
    return image


def c_image(name, image):
    pixels = []
    for r, g, b, a in image.getdata():
        pixels.extend((b, g, r, a))
    lines = [f'static const uint8_t icon_{name}_pixels[] = {{']
    for i in range(0, len(pixels), 24):
        lines.append('    ' + ', '.join(f'0x{v:02x}' for v in pixels[i:i+24]) + ',')
    lines += ['};', f'const lv_image_dsc_t badge_icon_{name} = {{',
              f'    .header = {{.magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_ARGB8888, .w = {SIZE}, .h = {SIZE}, .stride = {SIZE*4}}},',
              f'    .data_size = sizeof(icon_{name}_pixels),',
              f'    .data = icon_{name}_pixels,', '};', '']
    return '\n'.join(lines)


def main():
    OUT.mkdir(exist_ok=True)
    images = {name: create(name, art) for name, art in
              [('water', water), ('system', system)]}
    source = ['#include "lvgl.h"', '', '/* Original vector-style page icons; BGRA pixels in Flash. */', '']
    source += [c_image(name, image) for name, image in images.items()]
    (OUT / 'badge_page_icons.c').write_text('\n'.join(source))


if __name__ == '__main__':
    main()
