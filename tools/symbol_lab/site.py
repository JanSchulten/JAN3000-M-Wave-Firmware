# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 M-Vave-OP1 contributors
"""The symbol lab's data: OUT/cur (and OUT/prev) -> OUT/site/{data.json, sprites/*.png, prev/*.png} (tools/symbol_lab/build.sh)"""
import datetime, json, os, sys
from PIL import Image

out, ver = sys.argv[1], int(sys.argv[2])
here = os.path.dirname(os.path.abspath(__file__))
changes = json.load(open(os.path.join(here, 'changes.json'), encoding='utf-8'))
descs = json.load(open(os.path.join(here, 'descriptions.json'), encoding='utf-8'))
site = os.path.join(out, 'site')
os.makedirs(os.path.join(site, 'sprites'), exist_ok=True)
data = json.load(open(os.path.join(out, 'cur', 'data.json'), encoding='utf-8'))
def png(src, dst):
    Image.open(src).convert('P', palette=Image.ADAPTIVE, colors=64).save(dst, optimize=True)
for s in data:
    if s.get('group') in ('text', 'home') and s['label'] in descs:
        s['desc'] = descs[s['label']]
    cur = os.path.join(out, 'cur', s['id'] + '.ppm')
    png(cur, os.path.join(site, 'sprites', s['id'] + '.png'))
    prev = os.path.join(out, 'prev', s['id'] + '.ppm')
    if s.get('group') != 'home' and os.path.exists(prev) and open(prev, 'rb').read() != open(cur, 'rb').read():
        os.makedirs(os.path.join(site, 'prev'), exist_ok=True)
        png(prev, os.path.join(site, 'prev', s['id'] + '.png'))
        s['prev'] = True
        s['change'] = changes.get(s['id'], 'Geändert.')
json.dump({'version': ver, 'notes': changes.get('_notes', []), 'date': datetime.date.today().strftime('%d.%m.%Y'), 'symbols': data},
          open(os.path.join(site, 'data.json'), 'w', encoding='utf-8'), ensure_ascii=False)
