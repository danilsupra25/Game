#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор спрайт-листов ЭФФЕКТОВ «Пепла и Янтаря».

Все листы рисуются в градациях СЕРОГО: игра тонирует их в рантайме
цветом урона (tintFrame в Sprites.cpp), поэтому один лист работает
для огня, льда, яда, тайной магии и т.д.

Листы:
  fx_slash  - взмах клинка: anims slash(6) cleave(6) thrust(5), ячейка 96 (лог. 48)
  fx_claw   - когти: claw(5), ячейка 96
  fx_smash  - ударная волна: smash(6), ячейка 96
  fx_cast   - колдовской круг: cast(5), ячейка 96
  fx_arrow  - стрела: loop(4), ячейка 64 (лог. 32)
  fx_bolt   - сфера (огонь/тайна): loop(4), ячейка 64
  fx_shard  - ледяной осколок: loop(4), ячейка 64
  fx_zap    - молния: loop(4), ячейка 64
  fx_gob    - капля яда: loop(4), ячейка 64
  fx_soul   - душа (метка некроманта): loop(4), ячейка 64
  fx_rock   - камень: loop(4), ячейка 64

Запуск:  python3 tools/make_fx.py [outdir]
Результат: assets/sprites/fx_*.png + .json и tools/preview/fx_all.png
Все эффекты смотрят ВПРАВО (игра поворачивает кадр по направлению полёта/удара).
"""
import json
import math
import os
import sys

from PIL import Image, ImageDraw

# ---------------- оттенки серого (под тонировку) ----------------
W    = (252, 250, 246)      # раскалённое ядро
L1   = (215, 213, 210)
L2   = (168, 166, 164)
L3   = (112, 110, 110)
L4   = (66, 64, 66)


class C:
    def __init__(self, w, h):
        self.im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im)

    def px(self, x, y, c, a=255):
        x = int(round(x)); y = int(round(y))
        if 0 <= x < self.im.width and 0 <= y < self.im.height:
            self.im.putpixel((x, y), (c[0], c[1], c[2], a))

    def dot(self, x, y, r, c, a=255):
        """Мягкий кружок (кисть) - основа росчерков."""
        ri = int(math.ceil(r))
        for dy in range(-ri, ri + 1):
            for dx in range(-ri, ri + 1):
                d = math.hypot(dx, dy)
                if d <= r:
                    aa = int(a * (1.0 - 0.28 * (d / max(r, 0.001))))
                    self.px(x + dx, y + dy, c, min(255, aa))

    def line(self, x1, y1, x2, y2, c, w=1, a=255):
        n = max(2, int(math.hypot(x2 - x1, y2 - y1) * 2))
        for i in range(n + 1):
            t = i / n
            self.px(x1 + (x2 - x1) * t, y1 + (y2 - y1) * t, c, a)
        if w > 1:
            for i in range(n + 1):
                t = i / n
                self.dot(x1 + (x2 - x1) * t, y1 + (y2 - y1) * t, w * 0.5, c, a)

    def ring(self, cx, cy, rx, ry, c, w=1.0, a=255, a0=0.0, a1=360.0, n=90):
        steps = max(8, int(n * abs(a1 - a0) / 360.0))
        for i in range(steps + 1):
            ang = math.radians(a0 + (a1 - a0) * i / steps)
            x = cx + math.cos(ang) * rx
            y = cy + math.sin(ang) * ry
            if w > 1.2:
                self.dot(x, y, w * 0.5, c, a)
            else:
                self.px(x, y, c, a)

    def poly(self, pts, c, a=255):
        self.d.polygon([(int(round(x)), int(round(y))) for x, y in pts],
                       fill=(c[0], c[1], c[2], a))

    def ell(self, cx, cy, rx, ry, c, a=255):
        self.d.ellipse([int(cx - rx), int(cy - ry), int(cx + rx), int(cy + ry)],
                       fill=(c[0], c[1], c[2], a))


def fade_alpha(c, k):
    """Умножить альфу всего кадра на k (затухание)."""
    px = c.im.load()
    for y in range(c.im.height):
        for x in range(c.im.width):
            r, g, b, a = px[x, y]
            if a:
                px[x, y] = (r, g, b, max(0, min(255, int(a * k))))


# ============================================================
#  fx_slash: взмах клинка (логическая сетка 48)
# ============================================================
SL = 48

def slash_stroke(c, cx, cy, R, th0, th1, T, phase, k, flip=1):
    """Тонкий полумесяц: веретено толщины вдоль дуги, три СЛОЯ по очереди
    (иначе следующий шаг кисти стирает ядро предыдущего)."""
    n = 60
    span = (th1 - th0) * phase
    steps = max(2, int(n * phase))
    pts = []
    for i in range(steps + 1):
        u = i / steps
        th = th0 + span * u
        x = cx + math.cos(th) * R
        y = cy + math.sin(th) * R * 0.92
        prof = (math.sin(math.pi * u) ** 0.7) * (0.45 + 0.55 * u)
        wdt = T * (0.25 + 0.75 * prof)
        pts.append((x, y, wdt))
    for x, y, wdt in pts:
        c.dot(x, y, wdt + 1.1, L3, 120)
    for x, y, wdt in pts:
        c.dot(x, y, wdt, L1, 240)
    for x, y, wdt in pts:
        c.dot(x, y, max(0.55, wdt * 0.45), W, 255)
    th = th0 + span
    c.dot(cx + math.cos(th) * R, cy + math.sin(th) * R * 0.92, 1.6, W, 240)


def fx_slash_frame(i, anim):
    c = C(SL, SL)
    cx = cy = SL / 2.0 - 0.5
    k = i / 5.0 if anim != "thrust" else i / 4.0
    fade = 1.0 - max(0.0, (k - 0.55)) / 0.45 * 0.85
    if anim == "slash":
        R = 15.5 + k * 4.5
        phase = min(1.0, k * 1.9)
        slash_stroke(c, cx - 4, cy, R, math.radians(-118), math.radians(58), 2.3, phase, k)
        if k > 0.35:   # белый ведущий край
            th = math.radians(-118) + math.radians(176) * phase
            x = cx - 4 + math.cos(th) * (R + 0.5)
            y = cy + math.sin(th) * (R + 0.5) * 0.92
            c.dot(x, y, 2.2, W, 245)
    elif anim == "cleave":
        R = 18.5 + k * 5.0
        phase = min(1.0, k * 1.7)
        slash_stroke(c, cx - 5, cy + 1, R, math.radians(-132), math.radians(72), 3.0, phase, k)
        slash_stroke(c, cx - 2, cy - 2.5, R * 0.74, math.radians(-120), math.radians(60),
                     1.6, min(1.0, phase * 1.1), k)
        if k > 0.4:
            for s in range(3):
                th = math.radians(-132 + 204 * phase) + s * 0.16
                x = cx - 5 + math.cos(th) * (R + 1.5)
                y = cy + 1 + math.sin(th) * (R + 1.5) * 0.92
                c.px(x, y, W, 230)
    else:  # thrust - колющий выпад: узкий длинный след вправо
        L = 9.0 + k * 17.0
        wdt = (3.4 * math.sin(math.pi * min(1.0, k * 1.3)) + 0.8)
        c.dot(cx - 6, cy, 3.0, L3, 120)
        c.poly([(cx - 9, cy), (cx - 9 + L * 0.55, cy - wdt), (cx - 9 + L, cy),
                (cx - 9 + L * 0.55, cy + wdt)], L2, 150)
        c.poly([(cx - 7, cy), (cx - 7 + L * 0.55, cy - wdt * 0.5), (cx - 5 + L, cy),
                (cx - 7 + L * 0.55, cy + wdt * 0.5)], L1, 230)
        c.line(cx - 5, cy, cx - 3 + L * 0.96, cy, W, 1, 255)
        if k > 0.3:   # искра на острие
            c.dot(cx - 3 + L, cy, 2.2, W, 245)
            c.px(cx + L * 0.4, cy - 2.5, L1, 190)
            c.px(cx + L * 0.6, cy + 2.5, L1, 170)
    fade_alpha(c, fade)
    return c


# ============================================================
#  fx_claw: три когтя (логическая сетка 48)
# ============================================================
def fx_claw_frame(i):
    c = C(SL, SL)
    k = i / 4.0
    grow = min(1.0, k * 2.6)
    fade = 1.0 - max(0.0, (k - 0.5)) / 0.5
    layers = {120: [], 240: [], 255: []}
    for s_ in range(3):
        off = (s_ - 1) * 7.5 + (1 if s_ == 1 else 0)
        x0, y0 = 10.0, 24.0 + off - 3.5
        x1, y1 = 10.0 + 26.0 * grow, 24.0 + off + 4.5
        mx, my = (x0 + x1) * 0.5 + 2.0, (y0 + y1) * 0.5 - 4.0
        n = 26
        for j in range(int(n * grow) + 1):
            t = j / n
            x = (1 - t) * (1 - t) * x0 + 2 * (1 - t) * t * mx + t * t * x1
            y = (1 - t) * (1 - t) * y0 + 2 * (1 - t) * t * my + t * t * y1
            prof = math.sin(math.pi * min(1.0, t * 1.05)) ** 0.6
            r = (1.9 - 0.35 * abs(s_ - 1)) * (0.3 + 0.7 * prof)
            layers[120].append((x, y, r + 1.0))
            layers[240].append((x, y, r))
            layers[255].append((x, y, max(0.5, r * 0.45)))
    for (x, y, r) in layers[120]:
        c.dot(x, y, r, L3, 130)
    for (x, y, r) in layers[240]:
        c.dot(x, y, r, L1, 240)
    for (x, y, r) in layers[255]:
        c.dot(x, y, r, W, 252)
    # капли-искры с кончиков
    if k > 0.45:
        for s2, (dx, dy) in enumerate([(36, 30), (38, 24), (35, 18)]):
            c.px(dx + s2, dy - int((k - 0.45) * 10), L2, int(170 * fade))
    fade_alpha(c, max(0.0, fade))
    return c


def fx_smash_frame(i):
    c = C(SL, SL)
    k = i / 5.0
    cx = cy = SL / 2.0
    fade = 1.0 - k * 0.85
    if i == 0:   # вспышка в точке удара
        c.dot(cx, cy, 6.5, W, 230)
        c.dot(cx, cy, 10.0, L2, 110)
    r = 5.0 + k * 17.0
    w = max(1.0, 3.8 * (1.0 - k))
    c.ring(cx, cy + 3, r, r * 0.44, L1, w, int(220 * fade), n=130)
    c.ring(cx, cy + 3, r * 0.82, r * 0.36, W, max(0.9, w * 0.55), int(235 * fade), n=130)
    if k > 0.15:  # вторая волна
        r2 = 3.0 + (k - 0.15) * 15.0
        c.ring(cx, cy + 3, r2, r2 * 0.44, L3, max(0.8, w * 0.7), int(140 * fade), n=100)
    # осколки/комья земли
    for s in range(8):
        a = s * math.pi * 2 / 8 + 0.4
        d = (6.0 + k * 19.0) * (0.8 + 0.25 * ((s * 37) % 3) / 2.0)
        x = cx + math.cos(a) * d
        y = cy + 3 + math.sin(a) * d * 0.44 - k * (7.0 - s % 3 * 2.0)
        sz = 1.5 * (1.0 - k) + 0.5
        c.px(x, y, L1, int(220 * fade))
        if sz > 1:
            c.px(x + 1, y, L3, int(160 * fade))
            c.px(x, y + 1, L3, int(120 * fade))
    # трещины от центра
    if i < 3:
        for s in range(5):
            a = 0.7 + s * 1.25
            c.line(cx + math.cos(a) * 4, cy + 3 + math.sin(a) * 1.8,
                   cx + math.cos(a) * (9 + k * 8), cy + 3 + math.sin(a) * (4 + k * 3.5),
                   L4, 1, int(150 * (1 - k)))
    return c


# ============================================================
#  fx_cast: колдовской круг (логическая сетка 48)
# ============================================================
def fx_cast_frame(i):
    c = C(SL, SL)
    k = i / 4.0
    cx, cy = SL / 2.0, SL / 2.0 + 5
    grow = min(1.0, k * 2.0)
    fade = 1.0 - max(0.0, (k - 0.55)) / 0.45
    rot = k * 46.0
    r = 8.0 + grow * 9.5
    # эллипс на земле
    c.ring(cx, cy, r, r * 0.42, L1, 1.6, int(230 * fade), n=120)
    c.ring(cx, cy, r * 0.62, r * 0.26, L1, 1.0, int(170 * fade), n=90)
    # руны-зубцы по внешнему кольцу
    for s in range(8):
        a = math.radians(rot + s * 45.0)
        x = cx + math.cos(a) * r
        y = cy + math.sin(a) * r * 0.42
        c.px(x, y, W, int(240 * fade))
        c.px(x + math.cos(a) * 2.2, y + math.sin(a) * 0.9, L1, int(190 * fade))
    # вспышка в центре
    flash = math.sin(math.pi * min(1.0, k * 1.25))
    c.dot(cx, cy - 5, 2.2 + flash * 2.4, W, int(140 * flash * fade + 30))
    c.dot(cx, cy - 5, 1.1, W, int(255 * fade))
    # искры вверх
    for s in range(6):
        ph = k * 1.6 + s * 0.31
        x = cx + math.sin(ph * 7.0 + s * 2.0) * (4.0 + s)
        y = cy - 2 - (ph - int(ph)) * 16.0
        c.px(x, y, L1, int(200 * fade))
        c.px(x, y - 1, W, int(150 * fade))
    fade_alpha(c, max(0.15, fade))
    return c


# ============================================================
#  Снаряды (логическая сетка 32, ячейка 64)
# ============================================================
PR = 32
PC = PR / 2.0 - 0.5


def fx_arrow_frame(i):
    c = C(PR, PR)
    y = PC + [0, 0.6, -0.6, 0][i]        # лёгкое рыскание
    x0 = 5.0
    # след-полоса
    tl = 6.0 + (i % 2) * 2.0
    for t in range(int(tl)):
        a = int(150 * (1.0 - t / tl))
        c.px(x0 - 1 - t, y, L2, a)
        c.px(x0 - 1 - t, y - 1, L3, a // 2)
    # древко
    c.line(x0, y, x0 + 13, y, L1, 1, 255)
    c.line(x0 + 1, y - 0.6, x0 + 12, y - 0.6, L3, 1, 160)
    # наконечник
    c.poly([(x0 + 13, y - 2.4), (x0 + 18.5, y), (x0 + 13, y + 2.4), (x0 + 14.5, y)], W, 255)
    c.px(x0 + 17, y, W, 255)
    # оперение
    c.line(x0, y, x0 - 2.5, y - 2.6, L1, 1, 230)
    c.line(x0, y, x0 - 2.5, y + 2.6, L1, 1, 230)
    c.px(x0 - 1, y - 1, L3, 200)
    c.px(x0 - 1, y + 1, L3, 200)
    # блик скорости
    c.px(x0 + 8, y - 1, W, 170 + (i % 2) * 60)
    return c


def fx_bolt_frame(i):
    c = C(PR, PR)
    rot = i * math.pi / 2.0
    # хвост-вихрь слева
    for s in range(3):
        a = rot + s * 2.1
        tx = PC - 8.5 - s * 2.6
        ty = PC + math.sin(a) * 3.2
        c.dot(tx, ty, 3.4 - s * 0.7, L2, 150 - s * 30)
        c.dot(tx, ty, 1.8 - s * 0.4, L1, 190 - s * 35)
    # сфера
    c.dot(PC + 1, PC, 9.4, L3, 110)
    c.dot(PC + 1, PC, 7.6, L2, 170)
    c.dot(PC + 1, PC, 5.8, L1, 235)
    c.dot(PC + 1.8, PC - 0.8, 3.0, W, 255)
    c.px(PC - 0.6, PC + 2.2, W, 190)
    # вращающиеся прожилки
    for s in range(3):
        a = rot * 1.7 + s * 2.09
        x = PC + 1 + math.cos(a) * 6.6
        y = PC + math.sin(a) * 6.6
        c.px(x, y, W, 200)
        c.px(x - math.cos(a), y - math.sin(a), L1, 150)
    return c


def fx_shard_frame(i):
    c = C(PR, PR)
    y = PC + [0, 0.5, -0.5, 0][i]
    x = PC
    # след инея
    for t in range(6):
        a = int(140 * (1 - t / 6.0))
        c.px(x - 8 - t * 1.4, y + (t % 2), L2, a)
    # кристалл: вытянутый гексагоник вправо
    c.poly([(x - 10, y - 2.6), (x + 2, y - 4.2), (x + 11.5, y), (x + 2, y + 4.2), (x - 10, y + 2.6)], L1, 235)
    c.poly([(x - 7, y - 1.2), (x + 2, y - 2.2), (x + 10.0, y), (x + 2, y + 0.8), (x - 7, y + 0.5)], W, 230)
    c.line(x - 9, y, x + 10.5, y, W, 1, 200)
    c.px(x + 8, y, W, 255)
    # грани
    c.line(x - 2, y - 3.0, x - 3.4, y + 2.0, L3, 1, 170)
    # блёстки
    sp = [(x + 4, y - 4), (x - 4, y + 4), (x + 6, y + 3)]
    s = sp[i % 3]
    c.px(s[0], s[1], W, 235)
    c.px(s[0] + 1, s[1], L1, 150)
    c.px(s[0], s[1] + 1, L1, 150)
    return c


def fx_zap_frame(i):
    c = C(PR, PR)
    y = PC
    zig = [(-2.4, 2.6), (2.6, -2.4), (-2.0, -2.8), (2.2, 2.8)][i]
    pts = [(6, y)]
    x = 6.0
    seg = 0
    while x < 25:
        x += 3.4
        dy = zig[seg % 2] * (0.6 + 0.4 * ((i + seg) % 2))
        pts.append((x, y + dy))
        seg += 1
    pts.append((27, y))
    # непрерывное ядро по ломаной + мягкая юбка
    for j in range(len(pts) - 1):
        x1, y1, x2, y2 = pts[j][0], pts[j][1], pts[j + 1][0], pts[j + 1][1]
        steps = max(2, int(math.hypot(x2 - x1, y2 - y1) * 3))
        for s in range(steps + 1):
            t = s / steps
            px_ = x1 + (x2 - x1) * t
            py_ = y1 + (y2 - y1) * t
            c.dot(px_, py_, 1.5, L2, 130)
            c.dot(px_, py_, 0.9, L1, 220)
            c.px(px_, py_, W, 250)
    # свечение вокруг
    c.dot(PC + 1, y, 5.5, L3, 70)
    c.dot(25, y, 2.0, W, 235)
    c.dot(7, y, 1.6, W, 210)
    # ответвления
    bx = 12 + (i % 3) * 4
    c.line(bx, y - 1.5, bx + 2, y - 4.5, L1, 1, 200)
    c.line(bx + 3, y + 1.5, bx + 5, y + 4.0, L1, 1, 180)
    return c


def fx_gob_frame(i):
    c = C(PR, PR)
    rot = i * math.pi / 2.0
    y = PC + [0, 0.7, 0, -0.7][i]
    x = PC + 1
    # капля с хвостом влево: скруглённый контур из кистей
    n = 26
    for s in range(n + 1):
        a = math.pi * (-1.0 + 2.0 * s / n)          # обвод эллипса тела
        bx = x + math.cos(a) * 6.4
        by = y + math.sin(a) * 5.0
        c.dot(bx, by, 1.8, L2, 150)
    c.dot(x, y, 5.8, L1, 235)
    c.dot(x + 0.8, y, 4.6, L1, 250)
    c.dot(x + 1.8, y - 1.2, 2.4, W, 225)
    # хвост
    for t in range(4):
        c.dot(x - 7.0 - t * 2.4, y + (0.5 if t % 2 else -0.3), 3.0 - t * 0.55, L2, 150 - t * 25)
        c.dot(x - 7.0 - t * 2.4, y + (0.5 if t % 2 else -0.3), 1.5 - t * 0.25, L1, 180 - t * 30)
    # пузырьки на орбите
    for s in range(3):
        a = rot + s * 2.09
        bx = x + math.cos(a) * 5.8
        by = y + math.sin(a) * 4.6
        c.ring(bx, by, 1.6, 1.6, L1, 1, 210, n=14)
        c.px(bx - 0.5, by - 0.5, W, 220)
    return c


def fx_soul_frame(i):
    """Душа-череп с пламенным хвостом (метка/снаряды жреца пепла)."""
    c = C(PR, PR)
    y = PC + [0, -0.7, 0, 0.7][i]
    x = PC + 2
    # пламенный хвост: два язычка, колышутся
    for s in range(4):
        tx = x - 5.5 - s * 2.5
        ty = y + math.sin(i * 1.7 + s * 1.3) * 2.0
        c.dot(tx, ty, 3.0 - s * 0.55, L2, 160 - s * 26)
        c.dot(tx, ty, 1.6 - s * 0.3, L1, 200 - s * 34)
    c.dot(x - 6, y - 2 + (i % 2), 1.2, W, 160)
    # череп: купол + скулы + челюсть
    c.dot(x, y - 1.4, 4.6, L1, 255)
    c.dot(x - 2.8, y + 0.8, 2.5, L1, 255)
    c.dot(x + 2.8, y + 0.8, 2.5, L1, 255)
    c.ell(x, y + 3.2, 2.8, 1.9, L1, 255)
    c.px(x - 1, y + 3.4, L3, 210); c.px(x + 1, y + 3.4, L3, 210)
    # глазницы: тёмные провалы с тлеющим дном
    c.dot(x - 2.0, y - 1.0, 1.6, L4, 255)
    c.dot(x + 2.0, y - 1.0, 1.6, L4, 255)
    eg = W if i % 2 == 0 else L1
    c.px(x - 1.8, y - 0.6, eg, 240)
    c.px(x + 1.8, y - 0.6, eg, 240)
    c.px(x, y + 1.0, L3, 220)                 # нос
    c.dot(x - 1.0, y - 3.0, 1.2, W, 200)      # блик на лбу
    return c


def fx_rock_frame(i):
    c = C(PR, PR)
    rot = i * math.pi / 4.0
    pts = []
    for s in range(7):
        a = rot + s * 2 * math.pi / 7
        rr = 7.4 + (s * 29 % 3) * 1.1
        pts.append((PC + math.cos(a) * rr, PC + math.sin(a) * rr))
    c.poly(pts, L3, 255)
    c.poly(pts[:4] + [pts[0]], L2, 255)
    # освещённая грань сверху
    c.line(pts[0][0], pts[0][1], pts[1][0], pts[1][1], L1, 1, 255)
    c.line(pts[1][0], pts[1][1], pts[2][0], pts[2][1], L1, 1, 200)
    c.px(PC - 1.5, PC - 2, L1, 220)
    c.px(PC + 1, PC - 2.6, W, 160)
    # пыльный след
    c.px(PC - 8, PC + 2, L3, 150)
    c.px(PC - 10, PC + 3, L4, 110)
    return c


# ============================================================
#  сборка
# ============================================================
def build_sheet(frames, cols, key, fps, anims, log, out_px, ax, ay, outdir, outline_on=False):
    rows = (len(frames) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * out_px, rows * out_px), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        big = f.im.resize((out_px, out_px), Image.NEAREST)
        sheet.alpha_composite(big, ((i % cols) * out_px, (i // cols) * out_px))
    png = os.path.join(outdir, key + ".png")
    sheet.save(png)
    meta = {"key": key, "frameW": out_px, "frameH": out_px,
            "anchorX": ax, "anchorY": ay, "fps": fps, "anims": anims}
    with open(os.path.join(outdir, key + ".json"), "w", encoding="utf-8") as fh:
        json.dump(meta, fh, ensure_ascii=False, indent=1)
    print("%-9s %2d кадров -> %s" % (key, len(frames), png))
    return sheet


def make_preview(sheets, path, log_of):
    pad = 4
    rows = []
    for key, frames in sheets.items():
        log = log_of[key]
        sc = max(2, 96 // log)
        w = len(frames) * (log * sc + pad) + pad + 120
        rows.append((key, frames, log, sc, w))
    H = sum(l * s + pad + 14 for _, _, l, s, _ in rows) + pad
    W_ = max(w for *_, w in rows)
    out = Image.new("RGBA", (W_, H), (20, 17, 28, 255))
    d = ImageDraw.Draw(out)
    y = pad
    for key, frames, log, sc, _ in rows:
        d.text((pad, y), key, fill=(242, 181, 68, 255))
        y += 14
        for i, f in enumerate(frames):
            big = f.im.resize((log * sc, log * sc), Image.NEAREST)
            out.alpha_composite(big, (pad + i * (log * sc + pad), y))
        y += log * sc + pad
    out.save(path)
    print("превью ->", path)


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "assets", "sprites")
    os.makedirs(outdir, exist_ok=True)
    prevdir = os.path.join(root, "tools", "preview")
    os.makedirs(prevdir, exist_ok=True)

    sheets = {}
    log_of = {}

    # --- большие эффекты (лог. 48 -> ячейка 96, якорь в центре) ---
    big = {
        "fx_slash": ([fx_slash_frame(i, "slash") for i in range(6)] +
                     [fx_slash_frame(i, "cleave") for i in range(6)] +
                     [fx_slash_frame(i, "thrust") for i in range(5)],
                     {"slash": [0, 5], "cleave": [6, 11], "thrust": [12, 16]}, 18),
        "fx_claw": ([fx_claw_frame(i) for i in range(5)], {"claw": [0, 4]}, 18),
        "fx_smash": ([fx_smash_frame(i) for i in range(6)], {"smash": [0, 5]}, 16),
        "fx_cast": ([fx_cast_frame(i) for i in range(5)], {"cast": [0, 4]}, 16),
    }
    for key, (frames, anims, fps) in big.items():
        sheets[key] = frames
        log_of[key] = SL
        build_sheet(frames, 6, key, fps, anims, SL, 96, 48, 48, outdir)

    # --- снаряды (лог. 32 -> ячейка 64, якорь в центре) ---
    small = {
        "fx_arrow": ([fx_arrow_frame(i) for i in range(4)], 16),
        "fx_bolt":  ([fx_bolt_frame(i) for i in range(4)], 14),
        "fx_shard": ([fx_shard_frame(i) for i in range(4)], 16),
        "fx_zap":   ([fx_zap_frame(i) for i in range(4)], 20),
        "fx_gob":   ([fx_gob_frame(i) for i in range(4)], 12),
        "fx_soul":  ([fx_soul_frame(i) for i in range(4)], 14),
        "fx_rock":  ([fx_rock_frame(i) for i in range(4)], 10),
    }
    for key, (frames, fps) in small.items():
        sheets[key] = frames
        log_of[key] = PR
        build_sheet(frames, 4, key, fps, {"loop": [0, 3]}, PR, 64, 32, 32, outdir)

    make_preview(sheets, os.path.join(prevdir, "fx_all.png"), log_of)


if __name__ == "__main__":
    main()
