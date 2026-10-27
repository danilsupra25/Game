#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
slice_sheet.py - подготовка спрайт-листа для «Пепел и Янтарь».

Берёт сгенерированный/нарисованный лист с сеткой кадров, вырезает фон по цвету
(chroma-key), обрезает каждый кадр по габаритам персонажа и собирает НОВЫЙ лист
с одинаковыми кадрами, выровненными по ногам (якорь - низ-центр кадра).
Рядом кладёт .json с разметкой анимаций.

Пример:
  python3 tools/slice_sheet.py assets/sprites/example_player.png \
      --cols 4 --rows 2 --key FF00FF --frame 160 200 \
      --anims idle=0-3 walk=4-7 --fps 8

После этого файл example_player.png + example_player.json подхватятся игрой
(ключ = имя файла: player, e_scav, b_ash, mob3 ...).
"""
import argparse
import json
import os
from PIL import Image


def key_to_rgb(s):
    s = s.lstrip('#')
    return tuple(int(s[i:i + 2], 16) for i in (0, 2, 4))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('input')
    ap.add_argument('--cols', type=int, required=True)
    ap.add_argument('--rows', type=int, default=1)
    ap.add_argument('--key', default='auto', help='цвет фона для вырезания (auto - по углам листа)')
    ap.add_argument('--tol', type=int, default=60)
    ap.add_argument('--frame', nargs=2, type=int, default=None,
                    help='целевой размер кадра Ш В (по умолчанию - по макс. габариту)')
    ap.add_argument('--anims', nargs='*', default=None,
                    help='разметка: name=first-last [...]')
    ap.add_argument('--fps', type=float, default=10.0)
    ap.add_argument('--out', default=None, help='выходной png (по умолчанию - поверх входного)')
    a = ap.parse_args()

    im = Image.open(a.input).convert('RGB')
    W, H = im.size
    fw, fh = W // a.cols, H // a.rows
    if a.key == 'auto':
        cs = [px0 for px0 in [im.load()[0, 0], im.load()[W - 1, 0],
                              im.load()[0, H - 1], im.load()[W - 1, H - 1]]]
        kr = sum(c[0] for c in cs) // 4
        kg = sum(c[1] for c in cs) // 4
        kb = sum(c[2] for c in cs) // 4
        print('авто-ключ фона: #%02X%02X%02X' % (kr, kg, kb))
    else:
        kr, kg, kb = key_to_rgb(a.key)
    tol = a.tol

    px = im.load()
    frames = []
    for r in range(a.rows):
        for c in range(a.cols):
            x0, y0 = c * fw, r * fh
            # прозрачность по ключу + bbox
            box = None
            sub = Image.new('RGBA', (fw, fh), (0, 0, 0, 0))
            sp = sub.load()
            for y in range(fh):
                for x in range(fw):
                    rr, gg, bb = px[x0 + x, y0 + y]
                    if abs(rr - kr) <= tol and abs(gg - kg) <= tol and abs(bb - kb) <= tol:
                        continue
                    sp[x, y] = (rr, gg, bb, 255)
                    if box is None:
                        box = [x, y, x + 1, y + 1]
                    else:
                        box[0] = min(box[0], x); box[1] = min(box[1], y)
                        box[2] = max(box[2], x + 1); box[3] = max(box[3], y + 1)
            if box:
                sub = sub.crop(tuple(box))
                # despill: полутона фона на краях силуэта -> прозрачность
                sp2 = sub.load()
                for y in range(sub.height):
                    for x in range(sub.width):
                        rr, gg, bb, aa = sp2[x, y]
                        if aa and (rr + bb) / 2 - gg > 70 and rr > 90 and bb > 90:
                            sp2[x, y] = (rr, gg, bb, 0)
            frames.append(sub)

    if a.frame:
        FW, FH = a.frame
    else:
        FW = max(f.width for f in frames)
        FH = max(f.height for f in frames)

    maxW = max(f.width for f in frames)
    maxH = max(f.height for f in frames)
    sc = min(FW / float(maxW), FH / float(maxH))
    scaled = []
    for f in frames:
        nw = max(1, int(round(f.width * sc)))
        nh = max(1, int(round(f.height * sc)))
        scaled.append(f.resize((nw, nh), Image.LANCZOS) if (nw, nh) != (f.width, f.height) else f)
    out = Image.new('RGBA', (FW * a.cols, FH * a.rows), (0, 0, 0, 0))
    for i, f in enumerate(scaled):
        c, r = i % a.cols, i // a.cols
        # выравнивание по ногам (низ) и по центру
        dx = c * FW + (FW - f.width) // 2
        dy = r * FH + (FH - f.height)
        out.paste(f, (dx, dy), f)

    outpath = a.out or a.input
    out.save(outpath)

    meta = {
        'frameW': FW, 'frameH': FH,
        'anchorX': FW // 2, 'anchorY': FH - 2,
        'fps': a.fps,
        'anims': {},
    }
    if a.anims:
        for spec in a.anims:
            name, rng = spec.split('=')
            f0, f1 = rng.split('-')
            meta['anims'][name] = [int(f0), int(f1)]
    else:
        n = a.cols * a.rows - 1
        meta['anims'] = {'idle': [0, n], 'walk': [0, n]}
    jpath = os.path.splitext(outpath)[0] + '.json'
    with open(jpath, 'w', encoding='utf-8') as f:
        json.dump(meta, f, ensure_ascii=False, indent=2)
    print('OK ->', outpath, jpath, 'frame %dx%d, кадров %d' % (FW, FH, len(frames)))


if __name__ == '__main__':
    main()
