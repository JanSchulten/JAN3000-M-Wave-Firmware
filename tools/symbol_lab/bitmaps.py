# the PHYS MODEL pictures and STRC's cloud / bolt as 30 x 24 bitmaps -> C rows (bit 29 - x)
from PIL import Image, ImageDraw, ImageChops, ImageFilter
import math, sys
W, H = 30, 24
def new(): return Image.new('1', (W, H), 0)
def outline(m, w=1):
    er = m.convert('L')
    for _ in range(w): er = er.filter(ImageFilter.MinFilter(3))
    er = er.point(lambda p: 255 if p > 127 else 0).convert('1')
    return ImageChops.logical_and(m, ImageChops.invert(er))
def thick(m): return m.filter(ImageFilter.MaxFilter(3)) if False else m
def union(*ms):
    r = new()
    for m in ms: r = ImageChops.logical_or(r, m)
    return r
# human (MODAL): head + shoulders, outline
h = new(); d = ImageDraw.Draw(h)
d.ellipse((11, 0, 18, 7), fill=1)
d.rounded_rectangle((6, 9, 23, 26), radius=7, fill=1)
human = outline(h)
# strings (STRNG): three slanted lines, 2 px
s = new(); d = ImageDraw.Draw(s)
for (a, b) in (((1, 20), (23, 6)), ((7, 21), (26, 5)), ((15, 22), (28, 3))):
    d.line((a, b), fill=1, width=2)
strings = s
# sheep (MEMB): woolly body (circles), head, legs; outline of the wool, solid head and legs
w = new(); d = ImageDraw.Draw(w)
for (cx, cy, r) in ((8, 10, 4), (13, 7, 4.5), (18, 8, 4.5), (22, 11, 3.5), (18, 14, 4), (12, 14, 4), (7, 13, 3.5)):
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=1)
wool = outline(w)
hd = new(); d = ImageDraw.Draw(hd)
d.ellipse((22, 6, 28, 13), fill=1)          # head
d.line((9, 17, 9, 22), fill=1, width=2); d.line((19, 17, 19, 22), fill=1, width=2)   # legs
head = ImageChops.logical_and(hd, ImageChops.invert(w)) if False else hd
sheep = union(wool, head)
# vibrating strings (SYMP): the three strings with a wiggle across
v = new(); d = ImageDraw.Draw(v)
for k, ((x0, y0), (x1, y1)) in enumerate((((1, 20), (23, 6)), ((7, 21), (26, 5)), ((15, 22), (28, 3)))):
    L = math.hypot(x1 - x0, y1 - y0); nx, ny = -(y1 - y0) / L, (x1 - x0) / L
    pts = []
    for i in range(41):
        t = i / 40; a = 1.8 * math.sin(t * math.pi * 4 + k * 2) * math.sin(t * math.pi)
        pts.append((x0 + (x1 - x0) * t + nx * a, y0 + (y1 - y0) * t + ny * a))
    d.line(pts, fill=1, width=1)
symp = v
# STRC: a cloud (solid) on top, a bolt (solid) below it
c = new(); d = ImageDraw.Draw(c)
for (cx, cy, r) in ((9, 7, 4), (15, 5, 5), (21, 7, 4), (12, 9, 3), (18, 9, 3)):
    d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=1)
d.rectangle((6, 8, 24, 11), fill=1)
cloud = c
b = new(); d = ImageDraw.Draw(b)
d.polygon([(13, 11), (21, 11), (17, 16), (22, 16), (10, 24), (14, 18), (9, 18)], fill=1)
bolt = ImageChops.logical_and(b, ImageChops.invert(c))
# MRAT: a bell (the instrument): its outline, and its inside to fill
bl = new(); d = ImageDraw.Draw(bl)
d.ellipse((9, 1, 20, 12), fill=1)                       # the crown
d.polygon([(9, 7), (20, 7), (22, 15), (26, 19), (3, 19), (7, 15)], fill=1)   # the flaring body
d.ellipse((12, 18, 17, 23), fill=1)                     # the clapper
bell = outline(bl)
bellin = ImageChops.logical_and(bl, ImageChops.invert(bell))
pics = dict(human=human, strings=strings, sheep=sheep, symp=symp, cloud=cloud, bolt=bolt, bell=bell, bellin=bellin)
def rows(m): return [sum(1 << (29 - x) for x in range(W) if m.getpixel((x, y))) for y in range(H)]
if sys.argv[1] == 'png':
    sh = Image.new('L', ((W + 2) * len(pics), H), 40)
    for i, m in enumerate(pics.values()): sh.paste(m.convert('L').point(lambda p: 230 if p else 40), (i * (W + 2), 0))
    sh.resize((sh.width * 6, sh.height * 6), Image.NEAREST).save(sys.argv[2])
else:
    for n, m in pics.items():
        r = rows(m)
        print('static const uint32_t SYM_BM_%s[SYM_H] = {' % n.upper())
        for i in range(0, H, 6): print('    ' + ', '.join('%du' % v for v in r[i:i + 6]) + ',')   # (decimal: build.py reads hex words as addresses)
        print('};')
