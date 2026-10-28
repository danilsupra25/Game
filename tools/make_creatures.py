#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор спрайт-листов оставшихся существ и БОССОВ «Пепла и Янтаря».

Каждый пиксель рисуется программно в палитре существа (palFor из Sprites.cpp),
поэтому листы сочетаются с процедурной графикой игры. Всё оригинальное.

Мобы: кадр лог. 32 (ячейка 64). Боссы: кадр лог. 48 (ячейка 96).
Анимации: idle(4) walk(4) attack(4) attack2(4) [cast(4)|shoot(4)] death(4).
Существа смотрят ВПРАВО (игра зеркалит при движении влево).

Запуск: python3 tools/make_creatures.py [outdir]
"""
import json
import math
import os
import sys

from PIL import Image, ImageDraw

MOB_LOG = 32
MOB_OUT = 64
BOSS_LOG = 48
BOSS_OUT = 96

INK = (16, 14, 24)

# палитры существ (main, dark, light, accent, eye) - зеркалят Spr::palFor(tint)
PAL = {
    6:  ((72, 58, 96),  (28, 22, 40),  (126, 104, 160), (190, 140, 255), (240, 190, 255)),  # e_fang
    7:  ((96, 110, 66), (40, 48, 28),  (150, 164, 104), (210, 170, 90),  (255, 230, 140)),  # e_treant
    8:  ((120, 110, 70), (52, 46, 28), (180, 168, 110), (190, 230, 90),  (240, 255, 150)),  # e_hive
    9:  ((110, 140, 74), (46, 62, 30), (166, 196, 116), (200, 90, 120),  (255, 140, 160)),  # e_savage
    10: ((88, 116, 70), (34, 48, 26),  (146, 180, 116), (220, 120, 180), (255, 170, 220)),  # e_thornp
    12: ((140, 176, 206), (60, 86, 110), (206, 232, 250), (120, 200, 255), (200, 240, 255)),  # e_icehunt
    13: ((122, 158, 190), (48, 72, 100), (190, 220, 244), (140, 220, 255), (220, 250, 255)),  # e_golem
    14: ((150, 130, 206), (66, 52, 104), (210, 190, 250), (190, 150, 255), (240, 220, 255)),  # e_cspider
    15: ((120, 150, 190), (48, 66, 96), (186, 214, 244), (150, 220, 255), (230, 250, 255)),  # e_frostm
    16: ((222, 230, 238), (120, 134, 150), (250, 252, 255), (150, 210, 255), (210, 240, 255)),  # e_swolf
    18: ((96, 116, 58), (38, 48, 22),  (150, 172, 90),  (190, 240, 90),  (230, 255, 150)),  # e_rot
    19: ((150, 124, 104), (64, 50, 42), (200, 176, 152), (220, 80, 70),  (255, 150, 140)),  # e_devour
    20: ((84, 66, 120), (32, 24, 52),  (140, 116, 190), (200, 140, 255), (240, 200, 255)),  # e_morpri
    21: ((84, 102, 96), (32, 44, 42),  (140, 160, 152), (120, 200, 190), (200, 255, 245)),  # e_drown
    22: ((150, 160, 96), (64, 72, 38), (206, 214, 140), (200, 240, 90),  (240, 255, 160)),  # e_plague
    24: ((255, 140, 50), (150, 50, 20), (255, 210, 120), (255, 240, 180), (255, 255, 220)),  # e_elem
    25: ((120, 64, 44), (48, 22, 16),  (190, 110, 70),  (255, 150, 50),  (255, 220, 140)),  # e_forge
    26: ((120, 90, 180), (44, 28, 80), (180, 150, 240), (230, 180, 255), (255, 240, 255)),  # e_eye
    27: ((150, 110, 190), (60, 36, 90), (210, 170, 250), (255, 150, 255), (255, 220, 255)),  # e_warp
    28: ((70, 50, 120), (24, 16, 52),  (140, 110, 220), (220, 170, 255), (255, 230, 255)),  # e_horror
    # боссы
    5:  ((176, 74, 44), (66, 20, 14),  (255, 150, 80),  (255, 210, 110), (255, 240, 180)),  # b_ash
    11: ((110, 60, 80), (40, 20, 32),  (176, 110, 140), (255, 120, 170), (255, 190, 220)),  # b_thorn
    17: ((150, 196, 232), (58, 92, 128), (220, 240, 255), (140, 220, 255), (230, 250, 255)),  # b_frost
    23: ((104, 120, 58), (40, 50, 22), (160, 180, 96),  (190, 255, 90),  (235, 255, 160)),  # b_rot
    29: ((180, 120, 240), (50, 20, 90), (240, 200, 255), (255, 220, 120), (255, 255, 240)),  # b_star
}


class C:
    def __init__(self, w, h):
        self.im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im)

    def px(self, x, y, c, a=255):
        x = int(round(x)); y = int(round(y))
        if 0 <= x < self.im.width and 0 <= y < self.im.height:
            self.im.putpixel((x, y), (c[0], c[1], c[2], a))

    def r(self, x, y, w, h, c, a=255):
        if w <= 0 or h <= 0:
            return
        self.d.rectangle([int(x), int(y), int(x + w - 1), int(y + h - 1)],
                         fill=(c[0], c[1], c[2], a))

    def poly(self, pts, c, a=255):
        self.d.polygon([(int(round(a_)), int(round(b_))) for a_, b_ in pts],
                       fill=(c[0], c[1], c[2], a))

    def line(self, x1, y1, x2, y2, c, w=1, a=255):
        self.d.line([int(round(x1)), int(round(y1)), int(round(x2)), int(round(y2))],
                    fill=(c[0], c[1], c[2], a), width=int(max(1, round(w))))

    def ell(self, cx, cy, rx, ry, c, a=255):
        self.d.ellipse([int(cx - rx), int(cy - ry), int(cx + rx), int(cy + ry)],
                       fill=(c[0], c[1], c[2], a))

    def dot(self, cx, cy, r, c, a=255):
        self.ell(cx, cy, r, r, c, a)


def outline(c, col=INK):
    px = c.im.load()
    w, h = c.im.size
    add = []
    for y in range(h):
        for x in range(w):
            if px[x, y][3] == 0:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < w and 0 <= ny < h and px[nx, ny][3] > 0:
                        add.append((x, y)); break
    for x, y in add:
        px[x, y] = col + (255,)


def eyes(c, x, y, col, gap=2, blink=False):
    if blink:
        c.line(x, y, x, y, col, 1); c.line(x + gap, y, x + gap, y, col, 1)
    else:
        c.px(x, y, col); c.px(x + gap, y, col)


# ============================================================
#  ЗВЕРЬ (волк/клык): четвероногий силуэт
# ============================================================
def beast(c, P, x, y, s, legF=0, legB=0, jaw=0, tail=0, lunge=0, body=True):
    m, d, l, ac, ey = P
    lw = max(1, int(round(s)))
    # ноги (столбики в 2px)
    c.r(x - 5 * s + legB, y + 1 * s, lw + 1, 6 * s, d)
    c.r(x + 3 * s + legF, y + 1 * s, lw + 1, 6 * s, d)
    c.r(x - 1 * s + legF, y + 2 * s, lw + 1, 5 * s, m)
    c.r(x + 1 * s + legB, y + 2 * s, lw + 1, 5 * s, m)
    if not body:
        return
    # корпус
    c.ell(x + lunge, y, 6.5 * s, 3.4 * s, m)
    c.ell(x + lunge - 1 * s, y - 1.2 * s, 4.4 * s, 2.2 * s, l)
    c.ell(x + lunge + 1 * s, y + 1.4 * s, 4.0 * s, 1.6 * s, d)
    # хвост
    c.line(x - 6 * s + lunge, y - 1 * s, x - 8.5 * s + lunge - tail, y - 3.5 * s - tail, d, lw + 1)
    # голова
    hx = x + 6 * s + lunge
    c.ell(hx, y - 2 * s, 3.4 * s, 2.6 * s, m)
    c.ell(hx + 0.6 * s, y - 3.6 * s, 1.5 * s, 1.3 * s, l)      # ухо
    c.ell(hx - 1.4 * s, y - 3.4 * s, 1.2 * s, 1.1 * s, m)     # второе ухо
    # морда и пасть
    c.poly([(hx + 2 * s, y - 1.4 * s), (hx + 5.4 * s, y - 1 * s + jaw),
            (hx + 4.4 * s, y + 1.2 * s + jaw), (hx + 2 * s, y + 1.2 * s)], d)
    c.px(hx + 4.6 * s, y - 0.6 * s + jaw * 0.5, l)            # клык
    # глаза и brows
    eyes(c, hx - 0.5 * s, y - 2.2 * s, ey, max(2, int(s * 1.6)))
    c.line(hx - 1.4 * s, y - 3 * s, hx + 1.4 * s, y - 3 * s, d, 1)


# ============================================================
#  ГУМАНОИД: торс, ноги, руки, опц. оружие
# ============================================================
def humanoid(c, P, x, y, s, bob=0, legF=0, legB=0, armA=40, arm2A=200, lean=0, head=True):
    m, d, l, ac, ey = P
    c.line(x - 2 * s + legB, y + 1 * s, x - 2 * s + legB, y + 7 * s, d, max(1, s))
    c.line(x + 2 * s + legF, y + 1 * s, x + 2 * s + legF, y + 7 * s, d, max(1, s))
    c.poly([(x - 3 * s + lean, y - 5 * s + bob), (x + 3 * s + lean, y - 5 * s + bob),
            (x + 2 * s, y + 2 * s + bob), (x - 2 * s, y + 2 * s + bob)], m)
    c.line(x - 2 * s + lean, y - 4 * s + bob, x - 3 * s + lean, y - 2 * s + bob, l, max(1, s))
    if head:
        c.ell(x + lean, y - 7 * s + bob, 2.2 * s, 2.2 * s, l)
        eyes(c, x + lean, y - 7 * s + bob, ey, max(1, s))
    a = math.radians(armA)
    c.line(x + 2 * s + lean, y - 4 * s + bob, x + 2 * s + lean + math.cos(a) * 4 * s,
           y - 4 * s + bob + math.sin(a) * 4 * s, m, max(1, s))
    a2 = math.radians(arm2A)
    c.line(x - 2 * s + lean, y - 4 * s + bob, x - 2 * s + lean + math.cos(a2) * 3 * s,
           y - 4 * s + bob + math.sin(a2) * 3 * s, d, max(1, s))
    return (x + 2 * s + lean + math.cos(a) * 4 * s, y - 4 * s + bob + math.sin(a) * 4 * s)


# ============================================================
#  ПАУК: тело + 8 ног
# ============================================================
def spider(c, P, x, y, s, step=0, jaw=0, crest=0):
    m, d, l, ac, ey = P
    for k, lx in enumerate((-4, -2, 1, 3)):
        off = step if k % 2 == 0 else -step
        c.line(x + lx * s, y, x + lx * s - 3 * s + off, y + 6 * s, d, max(1, s))
        c.line(x + lx * s + 2 * s, y, x + lx * s + 5 * s - off, y + 6 * s, d, max(1, s))
    c.ell(x, y, 5 * s, 3.4 * s, m)
    c.ell(x - 1 * s, y - 1 * s, 3 * s, 2 * s, l)
    if crest:
        c.poly([(x - 2 * s, y - 3 * s), (x, y - 4.5 * s - crest), (x + 2 * s, y - 3 * s)], ac)
    c.ell(x + 5 * s, y - 1 * s, 2.4 * s, 2 * s, l)
    eyes(c, x + 5 * s, y - 1 * s, ey, max(1, s))
    c.line(x + 6 * s, y + jaw, x + 8 * s, y + 1 * s + jaw, d, max(1, s))
    c.line(x + 6 * s, y + 1 * s + jaw, x + 8 * s, y + jaw, d, max(1, s))


# ============================================================
#  ГОЛЕМ: блочное тело
# ============================================================
def golem(c, P, x, y, s, bob=0, armA=30, crack=0, legF=0, legB=0):
    m, d, l, ac, ey = P
    c.r(x - 3 * s + legB, y + 2 * s, 2 * s, 5 * s, d)
    c.r(x + 1 * s + legF, y + 2 * s, 2 * s, 5 * s, d)
    c.r(x - 4 * s, y - 5 * s + bob, 8 * s, 8 * s, m)
    c.r(x - 3 * s, y - 4 * s + bob, 6 * s, 3 * s, l)
    if crack:
        c.line(x - 2 * s, y - 3 * s + bob, x + 1 * s, y + 1 * s + bob, ac, max(1, s))
    a = math.radians(armA)
    c.r(x + 4 * s, y - 4 * s + bob + math.sin(a) * 2 * s, 3 * s, 4 * s, m)
    c.r(x - 7 * s, y - 4 * s + bob, 3 * s, 4 * s, d)
    c.ell(x, y - 6 * s + bob, 2.4 * s, 2 * s, l)
    eyes(c, x - 1 * s, y - 6 * s + bob, ey, max(1, s * 2))


# ============================================================
#  РОБА (маг/жрец): капюшон + посох
# ============================================================
def robed(c, P, x, y, s, bob=0, sway=0, staffA=90, orb=0, hood=True):
    m, d, l, ac, ey = P
    c.poly([(x - 4 * s + sway, y + 7 * s), (x + 4 * s + sway, y + 7 * s),
            (x + 2.4 * s, y - 5 * s + bob), (x - 2.4 * s, y - 5 * s + bob)], m)
    c.poly([(x - 3 * s + sway, y + 7 * s), (x - 1 * s, y + 7 * s),
            (x - 1.6 * s, y - 4 * s + bob), (x - 2.4 * s, y - 4 * s + bob)], d)
    if hood:
        c.poly([(x - 2.6 * s, y - 5 * s + bob), (x + 2.6 * s, y - 5 * s + bob),
                (x + 2 * s, y - 9 * s + bob), (x - 2 * s, y - 9 * s + bob)], l)
        c.r(x - 1.6 * s, y - 8 * s + bob, 3 * s, 2.4 * s, (24, 18, 28))
        eyes(c, x - 1 * s, y - 7 * s + bob, ey, max(1, s))
    a = math.radians(staffA)
    sx, sy = x + 4 * s, y - 3 * s + bob
    ex, eyy = sx + math.cos(a) * 8 * s, sy + math.sin(a) * 8 * s
    c.line(sx, sy + 3 * s, ex, eyy, (120, 92, 58), max(1, s))
    if orb:
        c.dot(ex, eyy, 1.6 * s * orb, ac)
        c.dot(ex, eyy, 0.8 * s * orb, ey)
    return (ex, eyy)


# ============================================================
#  СГUSTOK/БЛОБ: капля с глазами
# ============================================================
def blob(c, P, x, y, s, wob=0, mouths=0, bubbles=0):
    m, d, l, ac, ey = P
    c.ell(x, y + wob * 0.3, 6 * s, (4 + wob) * s, m)
    c.ell(x - 1 * s, y - 1 * s + wob * 0.3, 4 * s, (2.6 + wob * 0.6) * s, l)
    eyes(c, x - 1 * s, y - 1 * s, ey, max(1, s * 2))
    eyes(c, x + 2 * s, y - 1 * s, ey, max(1, s))
    if mouths:
        c.line(x - 2 * s, y + 1.5 * s, x + 2 * s, y + 1.5 * s + mouths, d, max(1, s))
    if bubbles:
        for k in range(bubbles):
            c.dot(x + (k * 3 - 3) * s, y - 4 * s - k, 1 * s, ac, 200)


# ============================================================
#  ГЛАЗ: парящее око со щупальцами
# ============================================================
def eyeball(c, P, x, y, s, hover=0, pupil=0, tent=0):
    m, d, l, ac, ey = P
    yy = y - 2 * s - hover
    for k in range(4):
        ph = tent + k
        c.line(x + (k - 1.5) * 2 * s, yy + 3 * s,
               x + (k - 1.5) * 2 * s + math.sin(ph) * 2 * s, yy + 7 * s, d, max(1, s))
    c.ell(x, yy, 5 * s, 4 * s, m)
    c.ell(x, yy, 3.4 * s, 3 * s, l)
    pr = 1.4 * s if pupil == 0 else pupil * s
    c.dot(x + 1 * s, yy, pr, (20, 12, 26))
    c.dot(x + 1.4 * s, yy - 0.4 * s, 0.5 * s, ey)
    c.line(x - 4 * s, yy - 2 * s, x + 4 * s, yy - 2 * s, ac, max(1, s))


# ============================================================
#  ЭЛЕМЕНТАЛЬ: пламенный силуэт
# ============================================================
def elemental(c, P, x, y, s, flick=0, armA=40, tall=0):
    m, d, l, ac, ey = P
    h = 8 * s + tall
    c.poly([(x - 3 * s, y + 6 * s), (x + 3 * s, y + 6 * s),
            (x + 2 * s, y - h * 0.4), (x, y - h + flick), (x - 2 * s, y - h * 0.4)], m)
    c.poly([(x - 1.6 * s, y + 5 * s), (x + 1.6 * s, y + 5 * s),
            (x + 1 * s, y - h * 0.3), (x, y - h * 0.7 - flick), (x - 1 * s, y - h * 0.3)], l)
    c.dot(x, y - h * 0.5, 1.2 * s, ac)
    a = math.radians(armA)
    c.line(x + 2 * s, y, x + 2 * s + math.cos(a) * 4 * s, y + math.sin(a) * 4 * s, m, max(1, s))
    c.line(x - 2 * s, y, x - 2 * s - math.cos(a) * 3 * s, y + math.sin(a) * 3 * s, d, max(1, s))
    eyes(c, x - 1 * s, y - h * 0.55, ey, max(1, s))


# ============================================================
#  УЖАС: клубок щупалец с глазами
# ============================================================
def horror(c, P, x, y, s, wave=0, eyesN=3, mouth=0):
    m, d, l, ac, ey = P
    c.ell(x, y, 5 * s, 4 * s, m)
    for k in range(6):
        a = math.radians(200 + k * 28)
        ph = wave + k
        c.line(x + math.cos(a) * 4 * s, y + math.sin(a) * 3 * s,
               x + math.cos(a) * 8 * s + math.sin(ph) * s, y + math.sin(a) * 6 * s + 2 * s,
               d, max(1, s))
    c.ell(x - 1 * s, y - 1.5 * s, 3 * s, 2.4 * s, l)
    for k in range(eyesN):
        eyes(c, x - 2 * s + k * 2 * s, y - 2 * s + (k % 2), ey, 1)
    if mouth:
        c.ell(x, y + 1.5 * s, 1.6 * s, mouth * s, (20, 10, 24))
    c.dot(x + 3 * s, y - 3 * s, 1 * s, ac, 220)


# ============================================================
#  РАСТЕНИЕ/ТРЕАНТ: ствол с ветвями
# ============================================================
def plant(c, P, x, y, s, sway=0, arms=0, bloom=0):
    m, d, l, ac, ey = P
    c.poly([(x - 3 * s, y + 7 * s), (x + 3 * s, y + 7 * s),
            (x + 2 * s + sway, y - 4 * s), (x - 2 * s + sway, y - 4 * s)], m)
    c.line(x - 1 * s, y + 6 * s, x - 1 * s + sway, y - 3 * s, d, max(1, s))
    for k in (-1, 1):
        aa = math.radians(60 if k > 0 else 120) + arms * 0.02 * k
        c.line(x + sway, y - 3 * s, x + sway + math.cos(aa) * 5 * s * k,
               y - 3 * s - math.sin(aa) * 4 * s - arms * 0.3, m, max(1, s))
    c.ell(x + sway, y - 6 * s, 3 * s, 2.6 * s, l)
    eyes(c, x + sway - 1 * s, y - 6 * s, ey, max(1, s))
    if bloom:
        c.dot(x + sway, y - 8.5 * s, 1.4 * s * bloom, ac)


# ============================================================
#  УЛЕЙ: соты с отверстиями
# ============================================================
def hive(c, P, x, y, s, pulse=0, open_=0):
    m, d, l, ac, ey = P
    c.ell(x, y + 1 * s, 5 * s, 5 * s + pulse, m)
    c.ell(x - 1 * s, y - 1 * s, 3 * s, 3 * s, l)
    for k, (ox, oy) in enumerate(((-2, -1), (1, 0), (-1, 2))):
        rr = (1.2 + open_ * 0.5) * s
        c.ell(x + ox * s, y + oy * s, rr, rr, (24, 18, 26))
        if open_ > 0:
            c.dot(x + ox * s, y + oy * s, rr * 0.5, ac, 200)
    # соты-шестиугольники намёком
    for k in range(3):
        c.line(x - 4 * s + k * 3 * s, y + 3 * s, x - 3 * s + k * 3 * s, y + 4 * s, d, 1)


# ============================================================
#  ОРУЖИЕ
# ============================================================
def blade(c, hx, hy, ang, L, m, l, ac):
    a = math.radians(ang)
    ca, sa = math.cos(a), math.sin(a)
    px_, py_ = -sa, ca
    c.line(hx - ca * 2, hy - sa * 2, hx + ca * L, hy + sa * L, m, 2)
    c.line(hx + ca * 2 + px_, hy + sa * 2 + py_, hx + ca * (L - 1) + px_, hy + sa * (L - 1) + py_, l, 1)
    c.px(hx + ca * L, hy + sa * L, l)
    c.px(hx + ca * (L + 1), hy + sa * (L + 1), l)
    c.line(hx - ca * 2 - px_ * 2, hy - sa * 2 + ca * -2 * -1, hx - ca * 2 + px_ * 2, hy - sa * 2 - ca * 2, ac, 1)
    c.px(hx - ca * 3.4, hy - sa * 3.4, ac)


def axe(c, hx, hy, ang, L, m, l, ac):
    a = math.radians(ang)
    ca, sa = math.cos(a), math.sin(a)
    px_, py_ = -sa, ca
    c.line(hx - ca * 2, hy - sa * 2, hx + ca * L, hy + sa * L, (120, 92, 58), 2)
    ex, ey = hx + ca * L, hy + sa * L
    c.poly([(ex - px_ * 1.5, ey - py_ * 1.5), (ex + ca * 3 - px_ * 5, ey + sa * 3 - py_ * 5),
            (ex + ca * 5 - px_ * 1.5, ey + sa * 5 - py_ * 1.5), (ex + ca * 3 + px_ * 5, ey + sa * 3 + py_ * 5),
            (ex + px_ * 1.5, ey + py_ * 1.5)], m)
    c.px(ex + ca * 4.5 - px_ * 4, ey + sa * 4.5 - py_ * 4, ac)


def bow(c, x, y, h, pull, m, l):
    for t in range(9):
        yy = y - h / 2 + h * t / 8
        xx = x - abs(math.sin(math.pi * t / 8)) * 3
        c.px(xx, yy, m)
    c.line(x - 2, y - h / 2, x - pull, y, l, 1)
    c.line(x - pull, y, x - 2, y + h / 2, l, 1)


# ============================================================
#  сборщик анимаций существа
# ============================================================
class Creature:
    def __init__(self, key, tint, log, out, kind, boss=False):
        self.key, self.tint, self.log, self.out = key, tint, log, out
        self.kind = kind          # "melee" | "ranged" | "caster"
        self.boss = boss
        self.P = PAL[tint]
        self.frames = []
        self.anims = {}

    def new(self):
        return C(self.log, self.log)

    def ground(self):
        return self.log - 5 if not self.boss else self.log - 6

    def add(self, anim, frames):
        a0 = len(self.frames)
        self.frames.extend(frames)
        self.anims[anim] = [a0, a0 + len(frames) - 1]

    def build(self, outdir):
        cols = 6 if not self.boss else 7
        rows = (len(self.frames) + cols - 1) // cols
        sheet = Image.new("RGBA", (cols * self.out, rows * self.out), (0, 0, 0, 0))
        for i, f in enumerate(self.frames):
            outline(f)
            big = f.im.resize((self.out, self.out), Image.NEAREST)
            sheet.paste(big, ((i % cols) * self.out, (i // cols) * self.out), big)
        png = os.path.join(outdir, self.key + ".png")
        sheet.save(png)
        scale = self.out // self.log
        anchor_y = (self.log - 5) if not self.boss else (self.log - 6)
        meta = {"key": self.key, "frameW": self.out, "frameH": self.out,
                "anchorX": self.out // 2, "anchorY": anchor_y * scale,
                "fps": 8 if not self.boss else 7, "anims": self.anims}
        with open(os.path.join(outdir, self.key + ".json"), "w", encoding="utf-8") as fh:
            json.dump(meta, fh, ensure_ascii=False, indent=1)
        print("%-9s %2d кадров -> %s" % (self.key, len(self.frames), png))
        return self.frames


# --------- универсальные наборы анимаций по архетипу ---------
def anims_melee(cr, bodyfn, atkfn, atk2fn):
    P = cr.P
    g = cr.ground()
    idle = []
    for i in range(4):
        c = cr.new(); bodyfn(c, bob=[0, -1, -1, 0][i], i=i, mode="idle"); idle.append(c)
    walk = []
    for i in range(4):
        c = cr.new(); bodyfn(c, bob=0, i=i, mode="walk",
                             legF=[2, 1, -1, -2][i], legB=[-2, -1, 1, 2][i]); walk.append(c)
    atk = []
    for i in range(4):
        c = cr.new(); atkfn(c, i); atk.append(c)
    atk2 = []
    for i in range(4):
        c = cr.new(); atk2fn(c, i); atk2.append(c)
    death = []
    for i in range(4):
        c = cr.new(); bodyfn(c, bob=0, i=i, mode="death", dk=i); death.append(c)
    cr.add("idle", idle); cr.add("walk", walk); cr.add("attack", atk)
    cr.add("attack2", atk2); cr.add("death", death)


def anims_ranged(cr, bodyfn, shootfn):
    g = cr.ground()
    idle = []
    for i in range(4):
        c = cr.new(); bodyfn(c, bob=[0, -1, -1, 0][i], i=i, mode="idle"); idle.append(c)
    walk = []
    for i in range(4):
        c = cr.new(); bodyfn(c, bob=0, i=i, mode="walk",
                             legF=[2, 1, -1, -2][i], legB=[-2, -1, 1, 2][i]); walk.append(c)
    shoot = []
    for i in range(4):
        c = cr.new(); shootfn(c, i); shoot.append(c)
    death = []
    for i in range(4):
        c = cr.new(); bodyfn(c, bob=0, i=i, mode="death", dk=i); death.append(c)
    cr.add("idle", idle); cr.add("walk", walk); cr.add("shoot", shoot); cr.add("death", death)


def anims_caster(cr, bodyfn, castfn):
    idle = []
    for i in range(4):
        c = cr.new(); bodyfn(c, bob=[0, -1, -1, 0][i], i=i, mode="idle"); idle.append(c)
    walk = []
    for i in range(4):
        c = cr.new(); bodyfn(c, bob=0, i=i, mode="walk",
                             legF=[2, 1, -1, -2][i], legB=[-2, -1, 1, 2][i]); walk.append(c)
    cast = []
    for i in range(4):
        c = cr.new(); castfn(c, i); cast.append(c)
    death = []
    for i in range(4):
        c = cr.new(); bodyfn(c, bob=0, i=i, mode="death", dk=i); death.append(c)
    cr.add("idle", idle); cr.add("walk", walk); cr.add("cast", cast); cr.add("death", death)


# ============================================================
#  МОБЫ
# ============================================================
def mob_fang(outdir):      # Теневой клык (зверь)
    cr = Creature("e_fang", 6, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            if k == 0:
                beast(c, P, 16, g - 6, 1, jaw=2)
            elif k == 1:
                c.ell(16, g - 2, 7, 3, P[0]); c.line(9, g - 1, 5, g - 3, P[1], 1)
                eyes(c, 20, g - 3, P[4], 2, blink=True)
            elif k == 2:
                c.ell(16, g - 1, 8, 2, P[1]); eyes(c, 21, g - 2, P[4], 2, blink=True)
            else:
                c.ell(16, g, 8, 1.4, P[1]); c.px(12, g - 2, P[3]); c.px(19, g - 3, P[3])
            return
        beast(c, P, 16, g - 6 + bob * 0.4, 1, legF, legB, tail=1 if i % 2 else 0)
    def atk(c, i):
        jaw = [0, 1, 3, 2][i]; lg = [0, -1, 2, 1][i]
        beast(c, P, 16 + lg, g - 6, 1, legF=[0, -1, 2, 1][i], jaw=jaw, lunge=lg)
    def atk2(c, i):
        beast(c, P, 16, g - 6 - [0, 2, 3, 1][i], 1, legF=[0, 1, 2, 0][i], jaw=[0, 0, 2, 3][i],
              lunge=[0, 0, 2, 3][i])
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_treant(outdir):    # Древесный страж (растение)
    cr = Creature("e_treant", 7, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            c.poly([(10, g), (22, g), (20, g - 6 + k * 2), (12, g - 6 + k * 2)], P[0])
            if k >= 1:
                c.line(13, g - 4, 10, g - 8 + k, P[1], 1); c.line(19, g - 4, 22, g - 8 + k, P[1], 1)
            if k >= 2:
                c.ell(16, g - 2, 6, 2, P[1]); c.px(12, g - 5, P[3]); c.px(20, g - 6, P[3])
            if k >= 3:
                c.ell(16, g - 1, 7, 1.6, P[1])
            return
        plant(c, P, 16 + legF * 0.3, g - 8 + bob * 0.4, 1, sway=bob * 0.5, arms=bob,
              bloom=1 if (mode == "idle" and i == 2) else 0)
    def atk(c, i):
        plant(c, P, 16, g - 8, 1, sway=[0, -2, 2, 1][i], arms=[0, 2, 6, 4][i])
        if i >= 2:
            c.line(20, g - 12, 25, g - 8, P[0], 2)
    def atk2(c, i):
        plant(c, P, 16, g - 8 - [0, 1, 2, 1][i], 1, sway=[0, 0, 1, 2][i], arms=[0, 1, 8, 6][i],
              bloom=1 if i >= 2 else 0)
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_hive(outdir):      # Ройный улей (турель)
    cr = Creature("e_hive", 8, MOB_LOG, MOB_OUT, "ranged")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            c.ell(16, g - 3, 6 - k, 5 - k, P[1] if k < 2 else P[0])
            if k >= 1:
                c.line(12, g - 5, 20, g - 1, (24, 18, 26), 1)
            if k >= 2:
                c.ell(16, g - 2, 6, 2, P[1])
            if k >= 3:
                c.ell(16, g - 1, 7, 1.2, P[1]); c.px(13, g - 3, P[3])
            return
        hive(c, P, 16, g - 5 + bob * 0.3, 1, pulse=bob * 0.3,
             open_=1 if (mode == "idle" and i % 2) else 0)
    def shoot(c, i):
        hive(c, P, 16, g - 5, 1, pulse=[0, 1, 1, 0][i], open_=[0, 1, 2, 1][i])
        if i >= 2:
            for k in range(2):
                c.px(24 + k * 3 + i, g - 8 - k * 2, P[3])
                c.px(25 + k * 3 + i, g - 7 - k * 2, P[4])
    anims_ranged(cr, body, shoot)
    return cr.build(outdir)


def mob_savage(outdir):    # Отравленный дикарь (гуманоид с копьём)
    cr = Creature("e_savage", 9, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            if k == 0:
                humanoid(c, P, 16, g - 8, 1, lean=-2)
            elif k == 1:
                humanoid(c, P, 16, g - 6, 1, lean=-3, head=True)
            elif k == 2:
                c.poly([(9, g - 2), (22, g - 2), (21, g), (10, g)], P[0])
                c.ell(22, g - 3, 2, 2, P[2])
            else:
                c.poly([(9, g - 1), (22, g - 1), (21, g), (10, g)], P[1])
                c.px(14, g - 3, P[3]); c.px(19, g - 3, P[3])
            return
        h = humanoid(c, P, 16, g - 8 + bob * 0.4, 1, bob=bob, legF=legF, legB=legB,
                     armA=35, lean=1 if i % 2 else 0)
        blade(c, h[0], h[1], -35, 8, (150, 124, 104), P[2], P[3])   # копьё/дубина
    def atk(c, i):
        h = humanoid(c, P, 16, g - 8, 1, armA=[-20, -60, 20, 60][i], lean=[0, -1, 2, 1][i],
                     legF=[0, -1, 2, 1][i])
        blade(c, h[0], h[1], [-30, -80, 10, 60][i], 9, (150, 124, 104), P[2], P[3])
    def atk2(c, i):
        h = humanoid(c, P, 16, g - 8 - [0, 1, 2, 1][i], 1, armA=[-90, -120, 30, 60][i],
                     lean=[0, -1, 2, 2][i], legF=[0, -1, 3, 2][i])
        blade(c, h[0], h[1], [-90, -120, 20, 60][i], 9, (150, 124, 104), P[2], P[3])
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_thornp(outdir):    # Жрица шипов (кастер)
    cr = Creature("e_thornp", 10, MOB_LOG, MOB_OUT, "caster")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            robed(c, P, 16, g - 8 + k * 2, 1, bob=0, sway=k, hood=k < 3, orb=0)
            if k >= 2:
                c.ell(16, g - 2, 6, 2, P[1])
            if k >= 3:
                c.ell(16, g - 1, 7, 1.4, P[1]); c.px(18, g - 4, P[3])
            return
        robed(c, P, 16, g - 8 + bob * 0.4, 1, bob=bob, sway=bob * 0.4, staffA=80,
              orb=0.6 if i % 2 else 0.4)
    def cast(c, i):
        o = [0.4, 0.8, 1.2, 0.8][i]
        sxy = robed(c, P, 16, g - 8, 1, bob=[0, 0, -1, 0][i], sway=[0, 1, 1, 0][i],
                    staffA=[70, 60, 40, 55][i], orb=o)
        if i >= 1:
            for a in range(0, 360, 60):
                c.px(sxy[0] + math.cos(math.radians(a)) * 3.4, sxy[1] + math.sin(math.radians(a)) * 3.4, P[3])
        if i >= 2:
            c.px(sxy[0] + 3, sxy[1] - 2, P[4]); c.px(sxy[0] - 3, sxy[1] - 2, P[4])
    anims_caster(cr, body, cast)
    return cr.build(outdir)


def mob_icehunt(outdir):   # Ледяной охотник (лучник)
    cr = Creature("e_icehunt", 12, MOB_LOG, MOB_OUT, "ranged")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            if k == 0:
                humanoid(c, P, 16, g - 8, 1, lean=-2)
            elif k == 1:
                humanoid(c, P, 16, g - 6, 1, lean=-3)
                bow(c, 24, g - 8, 10, 0, P[2], P[4])
            elif k == 2:
                c.poly([(9, g - 2), (22, g - 2), (21, g), (10, g)], P[0])
                bow(c, 24, g - 4, 9, 0, P[2], P[4])
            else:
                c.poly([(9, g - 1), (22, g - 1), (21, g), (10, g)], P[1])
                c.px(15, g - 3, P[3]); c.px(20, g - 4, P[4])
            return
        h = humanoid(c, P, 16, g - 8 + bob * 0.4, 1, bob=bob, legF=legF, legB=legB, armA=20)
        bow(c, 23, g - 11 + bob * 0.4, 11, 0, P[2], P[4])
    def shoot(c, i):
        pull = [1, 5, 0, 0][i]
        h = humanoid(c, P, 16, g - 8, 1, armA=[10, 10, 20, 20][i], legF=[1, 2, 2, 1][i],
                     legB=[-1, -2, -1, 0][i])
        bow(c, 24, g - 11, 11, pull, P[2], P[4])
        if i < 2:
            c.line(24 - pull, g - 11, 27, g - 11, (206, 232, 250), 1)
            c.px(27, g - 11, P[4])
        if i == 2:
            c.line(27, g - 11, 30, g - 11, (206, 232, 250), 1)
            c.px(30, g - 11, P[4])
    anims_ranged(cr, body, shoot)
    return cr.build(outdir)


def mob_golem(outdir):     # Морозный голем
    cr = Creature("e_golem", 13, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            golem(c, P, 16, g - 8 + k * 2, 1, crack=k)
            if k >= 2:
                c.r(10, g - 3, 12, 3, P[1])
            if k >= 3:
                c.r(9, g - 2, 14, 2, P[1]); c.px(13, g - 4, P[4]); c.px(20, g - 5, P[4])
            return
        golem(c, P, 16, g - 8 + bob * 0.4, 1, bob=bob, legF=legF, legB=legB,
              crack=1 if i == 2 else 0)
    def atk(c, i):
        golem(c, P, 16, g - 8, 1, armA=[20, -30, 40, 70][i], legF=[0, -1, 2, 1][i], crack=1)
    def atk2(c, i):
        golem(c, P, 16, g - 8 - [0, 1, 2, 1][i], 1, armA=[-60, -90, 60, 80][i],
              legF=[0, -1, 3, 2][i], crack=1)
        if i >= 2:
            c.line(12, g - 1, 20, g - 1, P[3], 1)
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_cspider(outdir):   # Кристальный паук
    cr = Creature("e_cspider", 14, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            spider(c, P, 16, g - 6 + k, 1, step=0, crest=k)
            if k >= 2:
                c.ell(16, g - 3, 6, 2, P[1])
            if k >= 3:
                c.ell(16, g - 2, 7, 1.4, P[1]); c.px(13, g - 4, P[3])
            return
        spider(c, P, 16, g - 6 + bob * 0.3, 1, step=(1 if i % 2 else 0), crest=1)
    def atk(c, i):
        spider(c, P, 16 + [0, -1, 2, 1][i], g - 6, 1, step=0, jaw=[0, 1, 3, 2][i], crest=1)
    def atk2(c, i):
        spider(c, P, 16, g - 6 - [0, 2, 3, 1][i], 1, step=0, jaw=[0, 0, 2, 3][i], crest=2)
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_frostm(outdir):    # Инейный чародей (кастер)
    cr = Creature("e_frostm", 15, MOB_LOG, MOB_OUT, "caster")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            robed(c, P, 16, g - 8 + k * 2, 1, sway=k, hood=k < 3)
            if k >= 2:
                c.ell(16, g - 2, 6, 2, P[1])
            if k >= 3:
                c.ell(16, g - 1, 7, 1.4, P[1]); c.px(18, g - 4, P[4])
            return
        robed(c, P, 16, g - 8 + bob * 0.4, 1, bob=bob, sway=bob * 0.4, staffA=85,
              orb=0.6 if i % 2 else 0.4)
    def cast(c, i):
        o = [0.4, 0.9, 1.3, 0.9][i]
        sxy = robed(c, P, 16, g - 8, 1, bob=[0, 0, -1, 0][i], sway=[0, 1, 1, 0][i],
                    staffA=[75, 60, 40, 60][i], orb=o)
        if i >= 1:
            for a in range(0, 360, 45):
                c.px(sxy[0] + math.cos(math.radians(a)) * 3.6, sxy[1] + math.sin(math.radians(a)) * 3.6, P[4])
    anims_caster(cr, body, cast)
    return cr.build(outdir)


def mob_swolf(outdir):     # Снежный волк (зверь, быстрый)
    cr = Creature("e_swolf", 16, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            if k == 0:
                beast(c, P, 16, g - 6, 1, jaw=2)
            elif k == 1:
                c.ell(16, g - 2, 7, 3, P[0]); eyes(c, 20, g - 3, P[4], 2, blink=True)
            elif k == 2:
                c.ell(16, g - 1, 8, 2, P[1])
            else:
                c.ell(16, g, 8, 1.4, P[1]); c.px(12, g - 2, P[3]); c.px(19, g - 3, P[2])
            return
        beast(c, P, 16, g - 6 + bob * 0.4, 1, legF, legB, tail=1 if i % 2 else 0)
    def atk(c, i):
        beast(c, P, 16 + [0, -1, 3, 2][i], g - 6, 1, legF=[0, -1, 3, 2][i],
              jaw=[0, 1, 3, 2][i], lunge=[0, -1, 3, 2][i])
    def atk2(c, i):
        beast(c, P, 16, g - 6 - [0, 3, 4, 2][i], 1, legF=[0, 2, 3, 1][i], jaw=[0, 0, 3, 3][i],
              lunge=[0, 1, 3, 3][i])
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_rot(outdir):       # Болотная гниль (блоб-кастер)
    cr = Creature("e_rot", 18, MOB_LOG, MOB_OUT, "caster")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            blob(c, P, 16, g - 4 + k, 1, wob=-k, bubbles=0)
            if k >= 2:
                c.ell(16, g - 1, 8, 1.6, P[1])
            if k >= 3:
                c.ell(16, g, 9, 1, P[1]); c.px(13, g - 2, P[3])
            return
        blob(c, P, 16, g - 4 + bob * 0.4, 1, wob=bob * 0.6, bubbles=1 if i % 2 else 0)
    def cast(c, i):
        blob(c, P, 16, g - 4, 1, wob=[0, 1, 2, 1][i], mouths=[0, 0, 1, 1][i], bubbles=i)
        if i >= 1:
            for a in range(0, 360, 60):
                c.px(16 + math.cos(math.radians(a + i * 20)) * 6, g - 8 + math.sin(math.radians(a + i * 20)) * 3, P[3])
    anims_caster(cr, body, cast)
    return cr.build(outdir)


def mob_devour(outdir):    # Трупоед (зверь с большой пастью)
    cr = Creature("e_devour", 19, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            if k == 0:
                beast(c, P, 16, g - 6, 1, jaw=3)
            elif k == 1:
                c.ell(16, g - 2, 7, 3, P[0]); eyes(c, 20, g - 3, P[4], 2, blink=True)
            elif k == 2:
                c.ell(16, g - 1, 8, 2, P[1])
            else:
                c.ell(16, g, 8, 1.4, P[1]); c.px(12, g - 2, P[3])
            return
        beast(c, P, 16, g - 6 + bob * 0.4, 1, legF, legB, jaw=1, tail=1 if i % 2 else 0)
    def atk(c, i):
        beast(c, P, 16 + [0, -1, 3, 2][i], g - 6, 1, legF=[0, -1, 3, 2][i],
              jaw=[1, 2, 4, 3][i], lunge=[0, -1, 3, 2][i])
    def atk2(c, i):
        beast(c, P, 16, g - 6 - [0, 2, 3, 1][i], 1, legF=[0, 1, 3, 1][i], jaw=[1, 1, 4, 4][i],
              lunge=[0, 0, 3, 3][i])
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_morpri(outdir):    # Жрец морока (кастер, парит)
    cr = Creature("e_morpri", 20, MOB_LOG, MOB_OUT, "caster")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            robed(c, P, 16, g - 8 + k * 2, 1, sway=k, hood=k < 3)
            if k >= 2:
                c.ell(16, g - 2, 6, 2, P[1])
            if k >= 3:
                c.ell(16, g - 1, 7, 1.4, P[1]); c.px(18, g - 4, P[3])
            return
        robed(c, P, 16, g - 8 + bob * 0.5, 1, bob=bob, sway=bob * 0.5, staffA=80,
              orb=0.6 if i % 2 else 0.4)
        # парит: нет ног, подол рваный
        c.line(12, g - 1, 13, g + 1, P[1], 1); c.line(19, g - 1, 18, g + 1, P[1], 1)
    def cast(c, i):
        o = [0.4, 0.9, 1.3, 0.9][i]
        sxy = robed(c, P, 16, g - 8, 1, bob=[0, 0, -1, 0][i], sway=[0, 1, 1, 0][i],
                    staffA=[70, 55, 35, 55][i], orb=o)
        if i >= 1:
            for a in range(0, 360, 45):
                c.px(sxy[0] + math.cos(math.radians(a + i * 15)) * 3.6,
                     sxy[1] + math.sin(math.radians(a + i * 15)) * 3.6, P[3])
    anims_caster(cr, body, cast)
    # собственный приём некроманта: «восстание» - посох вниз, из земли поднимается рука мертвеца
    raise_ = []
    staves = [75, 100, 125, 112]
    arms = [0, 2, 4, 3]
    glows = [0.3, 0.9, 1.4, 1.0]
    for i in range(4):
        c = cr.new()
        body(c, bob=-1 if i == 2 else 0, i=i, mode="idle")
        sxy = robed(c, P, 16, g - 8, 1, bob=-1 if i == 2 else 0, sway=0,
                    staffA=staves[i], orb=glows[i])
        hx = 16 + (i - 1) * 2.0   # рука мертвеца тянется вверх у ног жреца
        gy = g
        hgt = arms[i]
        if hgt > 0:
            col = P[2]
            c.line(hx, gy, hx, gy - hgt, col, 2)                 # предплечье
            c.line(hx, gy - hgt, hx - 1.4, gy - hgt - 1.6, col)  # большой палец
            c.line(hx, gy - hgt, hx + 0.2, gy - hgt - 2.0, col)  # пальцы
            c.line(hx, gy - hgt, hx + 1.6, gy - hgt - 1.6, col)
            c.ell(hx, gy + 0.5, 3.2 - i * 0.3, 1.0, P[1], 160)   # вскопанная земля
        if i >= 1:  # некротическое свечение от сферы к руке
            for k in range(3):
                t = (k + 1) / 4.0
                gx = sxy[0] + (hx - sxy[0]) * t
                gyy = sxy[1] + (gy - hgt - sxy[1]) * t
                c.px(gx, gyy, P[3], 200)
                c.px(gx + 0.6, gyy - 0.6, P[4], 140)
        raise_.append(c)
    cr.add("raise", raise_)
    return cr.build(outdir)


def mob_drown(outdir):     # Затонувший страж (голем-танк)
    cr = Creature("e_drown", 21, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            golem(c, P, 16, g - 8 + k * 2, 1, crack=k)
            if k >= 2:
                c.r(10, g - 3, 12, 3, P[1])
            if k >= 3:
                c.r(9, g - 2, 14, 2, P[1]); c.px(14, g - 4, P[4])
            return
        golem(c, P, 16, g - 8 + bob * 0.4, 1, bob=bob, legF=legF, legB=legB, crack=1 if i == 2 else 0)
        # водоросли/панцирь
        c.line(12, g - 12, 14, g - 14, P[3], 1)
    def atk(c, i):
        golem(c, P, 16, g - 8, 1, armA=[10, -40, 40, 70][i], legF=[0, -1, 2, 1][i], crack=1)
    def atk2(c, i):
        golem(c, P, 16, g - 8 - [0, 1, 2, 1][i], 1, armA=[-70, -100, 60, 80][i],
              legF=[0, -1, 3, 2][i], crack=1)
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_plague(outdir):    # Чумной стрелок (скелет-лучник)
    cr = Creature("e_plague", 22, MOB_LOG, MOB_OUT, "ranged")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            if k == 0:
                humanoid(c, P, 16, g - 8, 1, lean=-2)
            elif k == 1:
                c.r(12, g - 6, 8, 4, P[0]); c.ell(21, g - 6, 2, 2, P[2])
            elif k == 2:
                c.r(10, g - 3, 12, 3, P[1]); c.ell(22, g - 4, 2, 2, P[2])
            else:
                c.r(10, g - 2, 12, 2, P[1]); c.px(15, g - 4, P[3]); c.px(20, g - 4, P[4])
            return
        h = humanoid(c, P, 16, g - 8 + bob * 0.4, 1, bob=bob, legF=legF, legB=legB, armA=20)
        bow(c, 23, g - 11 + bob * 0.4, 11, 0, P[2], P[4])
    def shoot(c, i):
        pull = [1, 5, 0, 0][i]
        humanoid(c, P, 16, g - 8, 1, armA=[10, 10, 20, 20][i], legF=[1, 2, 2, 1][i],
                 legB=[-1, -2, -1, 0][i])
        bow(c, 24, g - 11, 11, pull, P[2], P[4])
        if i < 2:
            c.line(24 - pull, g - 11, 27, g - 11, P[3], 1)
            c.px(27, g - 11, P[4])
        if i == 2:
            c.line(27, g - 11, 30, g - 11, P[3], 1)
            c.px(30, g - 11, P[4])
    anims_ranged(cr, body, shoot)
    return cr.build(outdir)


def mob_elem(outdir):      # Огненный элементаль
    cr = Creature("e_elem", 24, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            elemental(c, P, 16, g - 4 + k, 1, flick=k, tall=-k * 2)
            if k >= 2:
                c.ell(16, g - 1, 6, 1.6, P[1])
            if k >= 3:
                c.px(13, g - 2, P[3]); c.px(18, g - 3, P[2]); c.px(20, g - 2, P[3])
            return
        elemental(c, P, 16, g - 4 + bob * 0.3, 1, flick=(2 if i % 2 else 0), armA=40)
    def atk(c, i):
        elemental(c, P, 16, g - 4, 1, flick=[0, 1, 3, 2][i], armA=[20, -20, 40, 70][i])
    def atk2(c, i):
        elemental(c, P, 16, g - 4 - [0, 1, 2, 1][i], 1, flick=[0, 1, 3, 2][i],
                  armA=[-60, -90, 50, 80][i], tall=[0, 1, 2, 1][i])
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_forge(outdir):     # Страж горнила (демон-кузнец с молотом)
    cr = Creature("e_forge", 25, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            golem(c, P, 16, g - 8 + k * 2, 1, crack=k)
            if k >= 2:
                c.r(10, g - 3, 12, 3, P[1])
            if k >= 3:
                c.r(9, g - 2, 14, 2, P[1]); c.px(14, g - 4, P[3]); c.px(19, g - 5, P[3])
            return
        h = golem(c, P, 16, g - 8 + bob * 0.4, 1, bob=bob, legF=legF, legB=legB,
                  crack=1 if i == 2 else 0)
    def atk(c, i):
        golem(c, P, 16, g - 8, 1, armA=[10, -50, 40, 70][i], legF=[0, -1, 2, 1][i], crack=1)
        # молот
        a = math.radians([10, -50, 40, 70][i])
        hx, hy = 20 + math.cos(a) * 6, g - 12 + math.sin(a) * 6
        c.r(hx - 1, hy - 2, 4, 4, P[1]); c.px(hx, hy, P[3])
    def atk2(c, i):
        golem(c, P, 16, g - 8 - [0, 1, 2, 1][i], 1, armA=[-70, -110, 60, 85][i],
              legF=[0, -1, 3, 2][i], crack=1)
        if i >= 2:
            c.line(11, g - 1, 21, g - 1, P[3], 2)
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_eye(outdir):       # Пустотный глаз (парящее око)
    cr = Creature("e_eye", 26, MOB_LOG, MOB_OUT, "ranged")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            eyeball(c, P, 16, g - 6 + k * 2, 1, hover=0, pupil=0)
            if k >= 2:
                c.ell(16, g - 3, 6, 2, P[1])
            if k >= 3:
                c.ell(16, g - 2, 7, 1.4, P[1]); c.px(16, g - 4, P[4])
            return
        eyeball(c, P, 16, g - 6 + bob * 0.5, 1, hover=bob, tent=i * 0.7)
    def shoot(c, i):
        pr = [0, 0.6, 2.2, 1.0][i]
        eyeball(c, P, 16, g - 6, 1, hover=[0, 1, 0, 0][i], pupil=pr, tent=i * 0.5)
        if i == 2:
            c.line(22, g - 10, 30, g - 10, P[4], 2)
    anims_ranged(cr, body, shoot)
    return cr.build(outdir)


def mob_warp(outdir):      # Искажённый (гуманоид-фаза)
    cr = Creature("e_warp", 27, MOB_LOG, MOB_OUT, "melee")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            humanoid(c, P, 16, g - 8 + k * 2, 1, lean=k)
            if k >= 2:
                for t in range(4):
                    c.px(12 + t * 2, g - 6 + k + (t % 2), P[3], 160)
            if k >= 3:
                c.ell(16, g - 2, 6, 1.6, P[1])
            return
        h = humanoid(c, P, 16, g - 8 + bob * 0.4, 1, bob=bob, legF=legF, legB=legB, armA=45)
        if i % 2:   # глитч-двойник
            for t in range(3):
                c.px(11 + t, g - 12 + t * 3, P[3], 120)
    def atk(c, i):
        h = humanoid(c, P, 16, g - 8, 1, armA=[0, -40, 30, 70][i], lean=[0, -1, 2, 1][i],
                     legF=[0, -1, 2, 1][i])
        blade(c, h[0], h[1], [-10, -60, 20, 60][i], 8, P[2], P[4], P[3])
    def atk2(c, i):
        h = humanoid(c, P, 16, g - 8 - [0, 2, 3, 1][i], 1, armA=[-80, -110, 40, 70][i],
                     lean=[0, -1, 3, 2][i], legF=[0, -1, 3, 2][i])
        blade(c, h[0], h[1], [-80, -110, 30, 60][i], 8, P[2], P[4], P[3])
    anims_melee(cr, body, atk, atk2)
    return cr.build(outdir)


def mob_horror(outdir):    # Звёздный ужас (клубок щупалец)
    cr = Creature("e_horror", 28, MOB_LOG, MOB_OUT, "caster")
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            horror(c, P, 16, g - 5 + k, 1, wave=k, eyesN=max(0, 3 - k), mouth=k * 0.4)
            if k >= 2:
                c.ell(16, g - 2, 7, 2, P[1])
            if k >= 3:
                c.ell(16, g - 1, 8, 1.4, P[1]); c.px(14, g - 3, P[4])
            return
        horror(c, P, 16, g - 5 + bob * 0.4, 1, wave=i * 0.6, eyesN=3)
    def cast(c, i):
        horror(c, P, 16, g - 5, 1, wave=i * 0.8, eyesN=3, mouth=[0, 0, 1, 1][i])
        if i >= 1:
            for a in range(0, 360, 45):
                c.px(16 + math.cos(math.radians(a + i * 20)) * 7, g - 10 + math.sin(math.radians(a + i * 20)) * 3, P[4])
    anims_caster(cr, body, cast)
    return cr.build(outdir)


# ============================================================
#  БОССЫ (лог. 48)
# ============================================================
def boss_ash(outdir):      # Патриарх Пепла - рогатый демон с клинком
    cr = Creature("b_ash", 5, BOSS_LOG, BOSS_OUT, "melee", boss=True)
    P = cr.P; g = cr.ground()

    def demon(c, bob=0, legF=0, legB=0, armA=40, lean=0, horns=True, big=1.6):
        s = big
        x, y = 24, g - 12 + bob * 0.4
        # ноги-столбы
        c.r(x - 4 * s + legB, y + 3 * s, int(2.4 * s), int(6.5 * s), P[1])
        c.r(x + 1.6 * s + legF, y + 3 * s, int(2.4 * s), int(6.5 * s), P[1])
        c.r(x - 4 * s + legB, y + 8.6 * s, int(3.2 * s), int(1.4 * s), P[0])
        c.r(x + 1.4 * s + legF, y + 8.6 * s, int(3.2 * s), int(1.4 * s), P[0])
        # торс-кираса
        c.poly([(x - 5 * s + lean, y - 5 * s), (x + 5 * s + lean, y - 5 * s),
                (x + 4 * s, y + 4 * s), (x - 4 * s, y + 4 * s)], P[0])
        c.poly([(x - 4 * s + lean, y - 4 * s), (x - 1 * s, y - 4 * s),
                (x - 1.5 * s, y + 3 * s), (x - 4 * s, y + 3 * s)], P[1])
        c.line(x - 4 * s, y + 1 * s, x + 4 * s, y + 1 * s, P[1], 2)          # пояс
        c.px(x, y + 1 * s, P[3]); c.px(x + 1, y + 1 * s, P[4])              # уголь-пряжка
        c.ell(x + 1.5 * s + lean, y - 1.5 * s, 1.5 * s, 1.8 * s, P[3])      # тлеющее ядро
        c.ell(x + 1.5 * s + lean, y - 1.5 * s, 0.7 * s, 0.9 * s, P[4])
        # наплечники-шипы
        c.poly([(x - 5.4 * s + lean, y - 5 * s), (x - 3 * s + lean, y - 5 * s),
                (x - 4.4 * s + lean, y - 7.4 * s)], P[1])
        c.poly([(x + 3 * s + lean, y - 5 * s), (x + 5.4 * s + lean, y - 5 * s),
                (x + 4.4 * s + lean, y - 7.4 * s)], P[1])
        # голова с личиной
        c.ell(x + lean, y - 8 * s, 3.4 * s, 3 * s, P[2])
        c.poly([(x - 2 * s + lean, y - 9 * s), (x + 2 * s + lean, y - 9 * s),
                (x + 1.4 * s + lean, y - 6 * s), (x - 1.4 * s + lean, y - 6 * s)], P[1])
        if horns:
            c.line(x - 2.8 * s + lean, y - 9.4 * s, x - 5.4 * s + lean, y - 13 * s, P[1], 2)
            c.line(x + 2.8 * s + lean, y - 9.4 * s, x + 5.4 * s + lean, y - 13 * s, P[1], 2)
            c.px(x - 5.4 * s + lean, y - 13.4 * s, P[3])
            c.px(x + 5.4 * s + lean, y - 13.4 * s, P[3])
        eyes(c, x + lean - 1.2 * s, y - 7.6 * s, P[4], max(2, int(s * 1.4)))
        a = math.radians(armA)
        hx = x + 4 * s + lean + math.cos(a) * 5 * s
        hy = y - 4 * s + math.sin(a) * 5 * s
        c.line(x + 4 * s + lean, y - 4 * s, hx, hy, P[0], 3)
        return (hx, hy)

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0):
        if mode == "death":
            k = dk
            h = demon(c, bob=k * 2, lean=k, horns=k < 3)
            if k >= 1:
                c.line(14, g - 4, 34, g - 4, P[3], 1)
            if k >= 2:
                c.ell(24, g - 4, 12, 4, P[1])
            if k >= 3:
                c.ell(24, g - 3, 14, 3, P[1])
                for t in range(5):
                    c.px(14 + t * 5, g - 8 - (t % 3) * 2, P[3], 200)
            return
        h = demon(c, bob=bob, legF=legF, legB=legB)
        blade(c, h[0], h[1], 62, 15, P[2], P[4], P[3])
    def atk(c, i):
        h = demon(c, armA=[0, -60, 40, 80][i], lean=[0, -2, 3, 2][i], legF=[0, -2, 4, 2][i])
        blade(c, h[0], h[1], [-10, -70, 30, 70][i], 18, P[2], P[4], P[3])
    def atk2(c, i):
        h = demon(c, bob=-[0, 2, 4, 2][i], armA=[-80, -120, 60, 90][i],
                  lean=[0, -2, 4, 3][i], legF=[0, -2, 5, 3][i])
        blade(c, h[0], h[1], [-80, -120, 40, 80][i], 18, P[2], P[4], P[3])
        if i >= 2:
            c.line(14, g - 2, 34, g - 2, P[3], 2)
    def cast(c, i):
        h = demon(c, armA=[-40, -70, -70, -50][i], lean=[0, -1, -1, 0][i])
        if i >= 1:
            for a in range(0, 360, 45):
                c.px(h[0] + math.cos(math.radians(a + i * 15)) * 6,
                     h[1] + math.sin(math.radians(a + i * 15)) * 6, P[3])
        if i >= 2:
            c.dot(h[0], h[1], 3, P[4])
    idle = [(_ := cr.new(), body(_ , bob=[0, -1, -1, 0][i], i=i))[0] for i in range(4)]
    walk = [(_ := cr.new(), body(_, legF=[3, 2, -2, -3][i], legB=[-3, -2, 2, 3][i], i=i))[0] for i in range(4)]
    atkf = [(_ := cr.new(), atk(_ , i))[0] for i in range(4)]
    atk2f = [(_ := cr.new(), atk2(_, i))[0] for i in range(4)]
    castf = [(_ := cr.new(), cast(_, i))[0] for i in range(4)]
    death = [(_ := cr.new(), body(_, i=i, mode="death", dk=i))[0] for i in range(4)]
    cr.add("idle", idle); cr.add("walk", walk); cr.add("attack", atkf)
    cr.add("attack2", atk2f); cr.add("cast", castf); cr.add("death", death)
    return cr.build(outdir)


def boss_thorn(outdir):    # Матка Терний - паук-растение
    cr = Creature("b_thorn", 11, BOSS_LOG, BOSS_OUT, "melee", boss=True)
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0, step=0, jaw=0, crest=1):
        if mode == "death":
            k = dk
            spider(c, P, 24, g - 10 + k * 2, 1.7, step=0, crest=k)
            if k >= 2:
                c.ell(24, g - 4, 11, 3, P[1])
            if k >= 3:
                c.ell(24, g - 3, 13, 2.4, P[1]); c.px(18, g - 7, P[3]); c.px(30, g - 8, P[3])
            return
        spider(c, P, 24, g - 10 + bob * 0.4, 1.7, step=step, jaw=jaw, crest=crest)
    def atk(c, i):
        body(c, step=0, jaw=[0, 1, 3, 2][i])
        spider(c, P, 24 + [0, -2, 4, 2][i], g - 10, 1.7, jaw=[0, 1, 3, 2][i], crest=1)
    def atk2(c, i):
        spider(c, P, 24, g - 10 - [0, 3, 5, 2][i], 1.7, jaw=[0, 0, 3, 3][i], crest=2)
    def cast(c, i):
        spider(c, P, 24, g - 10, 1.7, crest=2)
        if i >= 1:
            for a in range(0, 360, 45):
                c.px(24 + math.cos(math.radians(a + i * 15)) * 10,
                     g - 16 + math.sin(math.radians(a + i * 15)) * 5, P[3])
    idle = [(_ := cr.new(), body(_, bob=[0, -1, -1, 0][i], i=i, step=i % 2))[0] for i in range(4)]
    walk = [(_ := cr.new(), body(_, step=(i % 2), legF=i, legB=i))[0] for i in range(4)]
    atkf = [(_ := cr.new(), atk(_, i))[0] for i in range(4)]
    atk2f = [(_ := cr.new(), atk2(_, i))[0] for i in range(4)]
    castf = [(_ := cr.new(), cast(_, i))[0] for i in range(4)]
    death = [(_ := cr.new(), body(_, i=i, mode="death", dk=i))[0] for i in range(4)]
    cr.add("idle", idle); cr.add("walk", walk); cr.add("attack", atkf)
    cr.add("attack2", atk2f); cr.add("cast", castf); cr.add("death", death)
    return cr.build(outdir)


def boss_frost(outdir):    # Вьюговей - ледяной дух
    cr = Creature("b_frost", 17, BOSS_LOG, BOSS_OUT, "caster", boss=True)
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0, hover=0, staffA=80, orb=0):
        if mode == "death":
            k = dk
            robed(c, P, 24, g - 14 + k * 3, 1.7, sway=k, hood=k < 3, orb=0)
            if k >= 2:
                c.ell(24, g - 4, 10, 3, P[1])
            if k >= 3:
                c.ell(24, g - 3, 12, 2.4, P[1])
                for t in range(5):
                    c.px(14 + t * 5, g - 8 - (t % 3), P[4], 200)
            return
        sxy = robed(c, P, 24, g - 14 + bob * 0.5 - hover, 1.7, bob=bob, sway=bob * 0.5,
                    staffA=staffA, orb=orb)
        # корона-кристаллы
        c.line(20, g - 26 - hover, 21, g - 30 - hover, P[4], 1)
        c.line(24, g - 27 - hover, 24, g - 31 - hover, P[4], 1)
        c.line(28, g - 26 - hover, 27, g - 30 - hover, P[4], 1)
        return sxy
    def cast(c, i):
        o = [0.4, 0.9, 1.4, 0.9][i]
        sxy = body(c, bob=[0, 0, -1, 0][i], hover=[0, 1, 2, 1][i], staffA=[70, 55, 35, 55][i], orb=o)
        if sxy and i >= 1:
            for a in range(0, 360, 45):
                c.px(sxy[0] + math.cos(math.radians(a + i * 15)) * 6,
                     sxy[1] + math.sin(math.radians(a + i * 15)) * 6, P[4])
    def atk(c, i):
        body(c, staffA=[40, 0, 50, 80][i], hover=[0, 0, 1, 0][i])
    idle = [(_ := cr.new(), body(_, bob=[0, -1, -1, 0][i], i=i, hover=(1 if i % 2 else 0), orb=0.5))[0] for i in range(4)]
    walk = [(_ := cr.new(), body(_, bob=[0, -1, 0, -1][i], hover=(2 if i % 2 else 1), orb=0.4))[0] for i in range(4)]
    atkf = [(_ := cr.new(), atk(_, i))[0] for i in range(4)]
    castf = [(_ := cr.new(), cast(_, i))[0] for i in range(4)]
    death = [(_ := cr.new(), body(_, i=i, mode="death", dk=i))[0] for i in range(4)]
    cr.add("idle", idle); cr.add("walk", walk); cr.add("attack", atkf)
    cr.add("cast", castf); cr.add("death", death)
    return cr.build(outdir)


def boss_rot(outdir):      # Гнилостный Колосс - огромный блоб
    cr = Creature("b_rot", 23, BOSS_LOG, BOSS_OUT, "melee", boss=True)
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0, wob=0, mouths=0, armA=40):
        if mode == "death":
            k = dk
            blob(c, P, 24, g - 8 + k * 2, 1.8, wob=-k)
            if k >= 2:
                c.ell(24, g - 3, 13, 3, P[1])
            if k >= 3:
                c.ell(24, g - 2, 15, 2.4, P[1]); c.px(18, g - 6, P[3]); c.px(30, g - 7, P[3])
            return
        blob(c, P, 24, g - 8 + bob * 0.4, 1.8, wob=wob, mouths=mouths)
        a = math.radians(armA)
        c.line(32, g - 14, 32 + math.cos(a) * 8, g - 14 + math.sin(a) * 8, P[0], 3)
    def atk(c, i):
        body(c, wob=[0, 1, 2, 1][i], armA=[20, -30, 40, 70][i], mouths=[0, 0, 1, 1][i])
    def atk2(c, i):
        body(c, wob=[0, 2, 3, 2][i], armA=[-60, -100, 60, 90][i], mouths=[0, 1, 2, 1][i])
        if i >= 2:
            c.line(12, g - 2, 36, g - 2, P[3], 2)
    idle = [(_ := cr.new(), body(_, bob=[0, -1, -1, 0][i], i=i, mouths=(1 if i == 2 else 0)))[0] for i in range(4)]
    walk = [(_ := cr.new(), body(_, bob=[0, -1, 0, -1][i], wob=(1 if i % 2 else 0)))[0] for i in range(4)]
    atkf = [(_ := cr.new(), atk(_, i))[0] for i in range(4)]
    atk2f = [(_ := cr.new(), atk2(_, i))[0] for i in range(4)]
    death = [(_ := cr.new(), body(_, i=i, mode="death", dk=i))[0] for i in range(4)]
    cr.add("idle", idle); cr.add("walk", walk); cr.add("attack", atkf)
    cr.add("attack2", atk2f); cr.add("death", death)
    return cr.build(outdir)


def boss_star(outdir):     # Пожиратель Звёзд - космический ужас
    cr = Creature("b_star", 29, BOSS_LOG, BOSS_OUT, "caster", boss=True)
    P = cr.P; g = cr.ground()

    def body(c, bob=0, i=0, mode="idle", legF=0, legB=0, dk=0, wave=0, eyesN=3, mouth=0, hover=0):
        if mode == "death":
            k = dk
            horror(c, P, 24, g - 10 + k * 2, 1.8, wave=k, eyesN=max(0, 3 - k), mouth=k * 0.3)
            if k >= 2:
                c.ell(24, g - 4, 12, 3, P[1])
            if k >= 3:
                c.ell(24, g - 3, 14, 2.4, P[1])
                for t in range(6):
                    c.px(12 + t * 5, g - 9 - (t % 3) * 2, P[4], 200)
            return
        horror(c, P, 24, g - 10 + bob * 0.5 - hover, 1.8, wave=wave, eyesN=eyesN, mouth=mouth)
        # звезда-ядро
        c.dot(24, g - 14 - hover - bob * 0.5, 2.4, P[3])
        c.dot(24, g - 14 - hover - bob * 0.5, 1.2, P[4])
    def cast(c, i):
        body(c, wave=i * 0.8, eyesN=3, mouth=[0, 0, 1, 1][i], hover=[0, 1, 2, 1][i])
        if i >= 1:
            for a in range(0, 360, 30):
                c.px(24 + math.cos(math.radians(a + i * 20)) * 12,
                     g - 18 + math.sin(math.radians(a + i * 20)) * 6, P[4])
    def atk(c, i):
        body(c, wave=i * 0.6, mouth=[0, 1, 2, 1][i])
    idle = [(_ := cr.new(), body(_, bob=[0, -1, -1, 0][i], i=i, wave=i * 0.5, hover=(1 if i % 2 else 0)))[0] for i in range(4)]
    walk = [(_ := cr.new(), body(_, bob=[0, -1, 0, -1][i], wave=i * 0.7, hover=(2 if i % 2 else 1)))[0] for i in range(4)]
    atkf = [(_ := cr.new(), atk(_, i))[0] for i in range(4)]
    castf = [(_ := cr.new(), cast(_, i))[0] for i in range(4)]
    death = [(_ := cr.new(), body(_, i=i, mode="death", dk=i))[0] for i in range(4)]
    cr.add("idle", idle); cr.add("walk", walk); cr.add("attack", atkf)
    cr.add("cast", castf); cr.add("death", death)
    return cr.build(outdir)


def make_preview(sheets, path, logs):
    pad = 4
    rows = []
    for key, frames in sheets.items():
        log = logs[key]
        sc = max(2, 72 // log)
        w = len(frames) * (log * sc + pad) + pad + 140
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
            big = f.resize((log * sc, log * sc), Image.NEAREST)
            out.alpha_composite(big, (pad + i * (log * sc + pad), y))
            if i % 4 == 0:
                d.rectangle([pad + i * (log * sc + pad), y + log * sc + 1,
                             pad + i * (log * sc + pad) + 6, y + log * sc + 3],
                            fill=(255, 122, 46, 255))
        y += log * sc + pad
    out.save(path)
    print("превью ->", path)


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "assets", "sprites")
    os.makedirs(outdir, exist_ok=True)
    prevdir = os.path.join(root, "tools", "preview")
    os.makedirs(prevdir, exist_ok=True)

    mobs = [mob_fang, mob_treant, mob_hive, mob_savage, mob_thornp,
            mob_icehunt, mob_golem, mob_cspider, mob_frostm, mob_swolf,
            mob_rot, mob_devour, mob_morpri, mob_drown, mob_plague,
            mob_elem, mob_forge, mob_eye, mob_warp, mob_horror]
    bosses = [boss_ash, boss_thorn, boss_frost, boss_rot, boss_star]

    for fn in mobs + bosses:
        fn(outdir)

    # превью: нарезаем готовые листы на кадры
    prev = {}
    logs = {}
    for fn in mobs + bosses:
        nm = fn.__name__
        if nm.startswith("mob_"):
            k, lg = "e_" + nm[4:], MOB_LOG
        else:
            k, lg = "b_" + nm[5:], BOSS_LOG
        png = os.path.join(outdir, k + ".png")
        js = os.path.join(outdir, k + ".json")
        if not (os.path.exists(png) and os.path.exists(js)):
            continue
        img = Image.open(png)
        with open(js, encoding="utf-8") as fh:
            meta = json.load(fh)
        fw, fh_ = meta["frameW"], meta["frameH"]
        cols = img.width // fw
        frames = []
        for idx in range((img.width // fw) * (img.height // fh_)):
            cx, cy = (idx % cols) * fw, (idx // cols) * fh_
            frames.append(img.crop((cx, cy, cx + fw, cy + fh_)))
        prev[k] = frames
        logs[k] = lg
    make_preview(prev, os.path.join(prevdir, "creatures_all.png"), logs)


if __name__ == "__main__":
    main()
