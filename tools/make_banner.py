#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
# The README banner (docs/jan3000-banner.png) from the firmware's own UI renders in the PAPER palette.
# Run tests/run_tests.sh first (it renders build/ui_new/). fxtracks.png is FX TRACKS within the firmware's limits
# (FX1 LEN 32, FX2 LEN 24, a few punch-ins); the stock fxtracks128 scene still uses 128 steps. Then:
#   python3 tools/make_banner.py build/ui_new/PAPER docs/jan3000-banner.png "assets/fonts/InterTight[wght].ttf"
import sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter
R, OUT, FONT = sys.argv[1], sys.argv[2], sys.argv[3]   # build/ui_new/PAPER, out.png, InterTight[wght].ttf
W, H = 1440, 720
BG = (246, 238, 230)      # PAPER's page
CARD = (230, 218, 205)
LINE = (205, 194, 180)
INK = (32, 28, 24)
SUB = (110, 100, 92)
RED = (172, 56, 8)        # PAPER's accent
TEAL = (8, 85, 65)

def font(sz, w):
    f = ImageFont.truetype(FONT, sz)
    f.set_variation_by_axes([w])
    return f

img = Image.new("RGB", (W, H), BG)
d = ImageDraw.Draw(img)
for x in range(0, W, 40):                       # faint graph-paper grid
    d.line([(x, 0), (x, H)], fill=(239, 230, 220))
for y in range(0, H, 40):
    d.line([(0, y), (W, y)], fill=(239, 230, 220))

# --- left: the splash logo (same paper colour as the page), scaled up crisp
sp = Image.open(f"{R}/splash.png").convert("RGB")
logo = sp.crop((20, 60, 220, 182))
S = 2
logo = logo.resize((logo.width * S, logo.height * S), Image.NEAREST)
from PIL import ImageChops
mask = ImageChops.difference(logo, Image.new("RGB", logo.size, BG)).convert("L").point(lambda v: 255 if v > 3 else 0)
lx, ly = 70, 130
img.paste(logo, (lx, ly), mask)
d = ImageDraw.Draw(img)

ty = ly + logo.height + 34
d.text((lx, ty), "Custom firmware for the", font=font(30, 450), fill=SUB)
d.text((lx, ty + 36), "M-VAVE FM-1", font=font(56, 780), fill=INK)
d.text((lx, ty + 104), "OP-1 / OP-Z-inspired workflow", font=font(26, 560), fill=TEAL)

chips = ["SONG / SCENE MODE", "FX TRACKS", "MULTIPLE ENGINES", "J-6 CHORD MODE", "LIVE PUNCH-IN FX"]
cf = font(17, 680)
cx, cy = lx, ty + 160
for i, c in enumerate(chips):
    tw = d.textlength(c, font=cf)
    if cx + tw + 28 > 610:
        cx, cy = lx, cy + 40
    col = RED if i == 0 else TEAL
    d.rounded_rectangle([cx, cy, cx + tw + 24, cy + 30], radius=15, outline=col, width=2)
    d.text((cx + 12, cy + 6), c, font=cf, fill=col)
    cx += tw + 34

# --- right: six real screens, 3 x 2, as little devices with a soft shadow
shots = [("layer_seq_scenes", "SCENE MODE"), ("scene_song", "SONG MODE"), ("cset", "J-6 CHORD MODE"),
         ("fxtracks", "FX TRACKS"), ("perform_held", "LIVE PUNCH-IN FX"), ("layer_edit_active", "MULTIPLE ENGINES")]
sz, gap, lab = 240, 26, 34
gx0 = W - 70 - 3 * sz - 2 * gap
gy0 = (H - (2 * (sz + lab) + gap)) // 2
sh = Image.new("L", (W, H), 0)
sd = ImageDraw.Draw(sh)
for i in range(6):
    x = gx0 + (i % 3) * (sz + gap); y = gy0 + (i // 3) * (sz + lab + gap)
    sd.rounded_rectangle([x - 8, y - 2, x + sz + 8, y + sz + 14], radius=14, fill=70)
sh = sh.filter(ImageFilter.GaussianBlur(10))
img = Image.composite(Image.new("RGB", (W, H), (150, 132, 115)), img, sh)
d = ImageDraw.Draw(img)
lf = font(16, 720)
for i, (n, label) in enumerate(shots):
    x = gx0 + (i % 3) * (sz + gap)
    y = gy0 + (i // 3) * (sz + lab + gap)
    d.rounded_rectangle([x - 8, y - 8, x + sz + 8, y + sz + 8], radius=14, fill=INK)
    s = Image.open(f"{R}/{n}.png").convert("RGB")
    m = Image.new("L", (sz, sz), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, sz - 1, sz - 1], radius=7, fill=255)
    img.paste(s, (x, y), m)
    d.text((x - 6, y + sz + 16), label, font=lf, fill=INK)
    d.ellipse([x + sz - 4, y + sz + 20, x + sz + 4, y + sz + 28], fill=RED if i == 0 else TEAL)

img.save(OUT, optimize=True)
print(OUT, img.size)
