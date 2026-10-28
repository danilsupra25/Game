#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор оригинальных спрайт-листов «Пепла и Янтаря».

Рисует КАЖДЫЙ пиксель программно в палитре игры (Style.h), поэтому листы
гарантированно сочетаются с процедурной графикой. Никаких нейросетей и
чужих ассетов - всё оригинальное.

Кадр рисуется в логической сетке 32x32 и сохраняется в ячейку 64x64
(увеличение x2, nearest) - «плотный» пиксель-арт, который не плывёт
при игровом масштабировании.

Запуск:  python3 tools/make_sprites.py [outdir]
Результат: assets/sprites/<key>.png + <key>.json и tools/preview/*.png
Ключи: player, e_scav, e_arch, e_beetle, e_axer, e_dustm, npc0..npc3
Персонажи смотрят ВПРАВО (игра зеркалит кадр при движении влево).
"""
import json
import math
import os
import sys

from PIL import Image, ImageDraw

LOG = 32   # логический размер кадра
OUT = 64   # размер ячейки в листе

# ---------------- палитра (из src/Core/Style.h и тел существ) ----------------
INK      = (16, 14, 24)      # контур/бездна
INK2     = (26, 22, 36)
ASH      = (110, 104, 118)
ASH_D    = (74, 70, 82)
ASH_DD   = (52, 48, 60)
ARMOR    = (96, 88, 104)
ARMOR_L  = (146, 136, 154)
AMBER    = (242, 181, 68)
AMBER_HI = (255, 216, 140)
EMBER    = (255, 122, 46)
EMBER_HI = (255, 166, 96)
EMBER_D  = (176, 74, 48)
EMBER_DD = (116, 46, 32)
BONE     = (232, 220, 192)
BONE_D   = (168, 156, 134)
BONE_DD  = (120, 110, 92)
BLOOD    = (196, 48, 42)
SKIN     = (226, 196, 164)
SKIN_D   = (178, 144, 114)
DARK     = (40, 34, 44)
LEGS     = (58, 50, 62)
WOOD     = (120, 92, 58)
WOOD_D   = (84, 62, 38)
STEEL    = (158, 154, 166)
STEEL_D  = (104, 100, 112)
RUST     = (150, 110, 80)
RUST_D   = (104, 74, 52)
CLOTH    = (96, 104, 88)     # лохмотья падальщика
CLOTH_D  = (68, 74, 62)
DUST     = (122, 106, 94)    # роба прахового мага
DUST_D   = (88, 76, 68)
ROBE_P   = (120, 92, 150)    # торговец
ROBE_PD  = (86, 64, 112)
ROBE_G   = (90, 130, 110)    # целительница
ROBE_GD  = (62, 96, 80)
APRON    = (140, 96, 60)     # кузнец
APRON_D  = (100, 68, 42)
ROBE_B   = (96, 104, 132)    # старейшина
ROBE_BD  = (68, 74, 98)
WHITE    = (236, 232, 222)
GREY_BEARD = (190, 186, 178)


# ---------------- холст и примитивы ----------------
class C:
    def __init__(self, w=LOG, h=LOG):
        self.im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im)

    def px(self, x, y, c):
        x = int(round(x)); y = int(round(y))
        if 0 <= x < self.im.width and 0 <= y < self.im.height:
            self.im.putpixel((x, y), c if len(c) == 4 else c + (255,))

    def r(self, x, y, w, h, c):
        if w <= 0 or h <= 0:
            return
        self.d.rectangle([x, y, x + w - 1, y + h - 1], fill=c + (255,) if len(c) == 3 else c)

    def poly(self, pts, c):
        self.d.polygon([(int(round(a)), int(round(b))) for a, b in pts],
                       fill=c + (255,) if len(c) == 3 else c)

    def line(self, x1, y1, x2, y2, c, w=1):
        self.d.line([int(round(x1)), int(round(y1)), int(round(x2)), int(round(y2))],
                    fill=c + (255,) if len(c) == 3 else c, width=w)

    def ell(self, cx, cy, rx, ry, c):
        self.d.ellipse([int(cx - rx), int(cy - ry), int(cx + rx), int(cy + ry)],
                       fill=c + (255,) if len(c) == 3 else c)

    def copy(self):
        n = C(self.im.width, self.im.height)
        n.im = self.im.copy()
        n.d = ImageDraw.Draw(n.im)
        return n


def outline(c, col=INK):
    """Внешний контур 1px вокруг силуэта (классика пиксель-арта)."""
    px = c.im.load()
    w, h = c.im.size
    add = []
    for y in range(h):
        for x in range(w):
            if px[x, y][3] == 0:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < w and 0 <= ny < h and px[nx, ny][3] > 0:
                        add.append((x, y))
                        break
    for x, y in add:
        px[x, y] = col + (255,)


def arcpts(cx, cy, r, a0, a1, n=16):
    out = []
    for i in range(n + 1):
        a = math.radians(a0 + (a1 - a0) * i / n)
        out.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
    return out


def trail2(c, cx, cy, r, a0, a1, col, col2, n=18):
    for i in range(n + 1):
        a = math.radians(a0 + (a1 - a0) * i / n)
        c.px(cx + math.cos(a) * r, cy + math.sin(a) * r, col)
        c.px(cx + math.cos(a) * (r - 1), cy + math.sin(a) * (r - 1), col2)


def sparks(c, x, y, col, n=5, seed=1):
    rnd = seed
    for i in range(n):
        rnd = (rnd * 1103515245 + 12345) & 0x7FFFFFFF
        dx = (rnd % 7) - 3
        rnd = (rnd * 1103515245 + 12345) & 0x7FFFFFFF
        dy = -((rnd % 4) + 1)
        c.px(x + dx, y + dy, col)


def dustp(c, x, y, n=3, col=BONE_DD):
    for i in range(n):
        c.px(x - 2 - i * 2, y - (i % 2), col)


# ---------------- части тел ----------------
def sword(c, hx, hy, ang, L=11, blade=BONE, edge=AMBER_HI, grip=DARK):
    """Меч из руки (hx,hy) под углом ang (0=вправо, 90=вниз)."""
    a = math.radians(ang)
    ca, sa = math.cos(a), math.sin(a)
    tx, ty = hx + ca * L, hy + sa * L
    c.line(hx + ca * 2, hy + sa * 2, tx, ty, blade, 2)
    c.px(tx, ty, edge)                       # остриё
    c.px(tx - ca, ty - sa, edge)
    gx, gy = hx + ca * 2.5, hy + sa * 2.5    # гарда
    c.line(gx - sa * 2, gy + ca * 2, gx + sa * 2, gy - ca * 2, AMBER, 1)
    c.line(hx - ca * 2, hy - sa * 2, hx + ca * 2, hy + sa * 2, grip, 2)  # рукоять
    c.px(hx - ca * 3, hy - sa * 3, AMBER)    # навершие


def axe(c, hx, hy, ang, shaft=13):
    """Топор: длинное древко + широкая двойная голова."""
    a = math.radians(ang)
    ca, sa = math.cos(a), math.sin(a)
    ex, ey = hx + ca * shaft, hy + sa * shaft
    c.line(hx - ca * 3, hy - sa * 3, ex, ey, WOOD_D, 2)
    # голова топора (у дальнего конца), перпендикулярно древку
    px_, py_ = -sa, ca
    c.poly([(ex - px_ * 1.5, ey - py_ * 1.5),
            (ex + ca * 3 - px_ * 5.2, ey + sa * 3 - py_ * 5.2),
            (ex + ca * 5.4 - px_ * 1.4, ey + sa * 5.4 - py_ * 1.4),
            (ex + ca * 3 + px_ * 5.2, ey + sa * 3 + py_ * 5.2),
            (ex + px_ * 1.5, ey + py_ * 1.5)], STEEL_D)
    c.px(ex + ca * 4.8 - px_ * 4.2, ey + sa * 4.8 - py_ * 4.2, AMBER)
    c.px(ex + ca * 5.4, ey + sa * 5.4, AMBER_HI)
    c.px(ex + ca * 4.8 + px_ * 4.2, ey + sa * 4.8 + py_ * 4.2, AMBER)


def bowv(c, x, y, h=13, pull=0, col=WOOD):
    """Лук (вертикальная дуга), тетива оттянута на pull px влево."""
    for t in range(9):
        yy = y - h / 2 + h * t / 8
        xx = x - abs(math.sin(math.pi * t / 8)) * 3
        c.px(xx, yy, col)
        c.px(xx + 1 if t in (0, 8) else xx, yy, WOOD_D if t in (0, 8) else col)
    c.line(x - 2, y - h / 2, x - pull, y, BONE, 1)
    c.line(x - pull, y, x - 2, y + h / 2, BONE, 1)


def eyes2(c, x, y, col=AMBER_HI):
    c.px(x, y, col)
    c.px(x + 2, y, col)


# ============================================================
#  ГЕРОЙ: Пепельный странник
#  капюшон + тлеющий плащ + клинок. Оригинальный силуэт игры.
# ============================================================
def pl_base(bob=0, cloak=0, legF=0, legB=0, lean=0):
    """Плащ, ноги, торс, капюшон. Возвращает холст + смещение bob."""
    c = C()
    by = bob
    # задняя нога (за плащом)
    c.r(13 + legB, 21 + by, 2, 4, (74, 66, 82))
    c.r(13 + legB, 25 + by, 2, 2, (44, 38, 52))
    # плащ (рваный подол)
    c.poly([(11, 12 + by), (9 - cloak, 18 + by), (8 - cloak, 23 + by),
            (10 - cloak, 24 + by), (12, 22 + by), (14, 24 + by),
            (16, 22 + by), (17, 19 + by), (16, 12 + by)], EMBER_D)
    c.poly([(9 - cloak, 18 + by), (8 - cloak, 23 + by), (10 - cloak, 24 + by), (11, 21 + by)], EMBER_DD)
    c.px(10 - cloak, 24 + by, EMBER_DD)      # рванина
    c.px(14, 24 + by, EMBER_DD)
    # передняя нога (поверх плаща - читается в любой темноте)
    c.r(17 + legF, 21 + by, 2, 4, (86, 78, 94))
    c.r(17 + legF, 25 + by, 2, 2, (48, 42, 56))
    c.px(17 + legF, 25 + by, BONE_D)
    # торс
    c.r(12 + lean, 13 + by, 8, 8, ARMOR)
    c.r(18 + lean, 13 + by, 2, 8, ARMOR_L)
    c.r(12 + lean, 20 + by, 8, 1, DARK)      # пояс
    c.px(16 + lean, 20 + by, AMBER)          # пряжка-уголёк
    c.px(13 + lean, 14 + by, ARMOR_L)        # наплечник
    c.px(14 + lean, 14 + by, ARMOR_L)
    # капюшон
    c.poly([(11 + lean, 5 + by), (19 + lean, 5 + by), (20 + lean, 9 + by),
            (19 + lean, 13 + by), (11 + lean, 13 + by), (10 + lean, 8 + by)], ASH_D)
    c.poly([(10 + lean, 8 + by), (11 + lean, 5 + by), (13 + lean, 5 + by), (11 + lean, 9 + by)], ASH_DD)
    c.r(16 + lean, 8 + by, 3, 4, (28, 22, 32))       # тень лица
    eyes2(c, 16 + lean, 9 + by, AMBER_HI)            # тлеющие глаза
    return c, by


def pl_arm_sword(c, ang, by=0, hx=21, hy=18, L=11, armcol=ARMOR):
    """Передняя рука + меч под углом ang."""
    a = math.radians(ang)
    ex, ey = hx + math.cos(a) * 2, hy + math.sin(a) * 2
    c.line(19, 15 + by, ex, ey + by, armcol, 2)
    sword(c, hx, hy + by, ang, L)


def pl_idle(i):
    bob = [0, -1, -1, 0][i]
    cloak = [0, 1, 0, -1][i]
    c, by = pl_base(bob=bob, cloak=cloak)
    pl_arm_sword(c, 118 - bob * 2, by)
    if i == 2:
        c.px(20, 12 + by, AMBER_HI)          # блик на клинке
    return c


def pl_walk(i):
    phases = [(2, -2, 0, 1), (1, -1, -1, 2), (0, 0, -1, 0),
              (-1, 1, 0, 1), (-2, 2, -1, 2), (-1, 1, -1, 0)]
    legF, legB, bob, cloak = phases[i]
    c, by = pl_base(bob=bob, cloak=cloak, legF=legF, legB=legB)
    pl_arm_sword(c, 112 + legF * 3, by)
    # задняя рука качается
    c.line(13, 15 + by, 11 - legF, 19 + by, ARMOR, 2)
    if i in (1, 4):
        dustp(c, 12 - legF, 27, 2)
    return c


def pl_attack(i):
    """Горизонтальный рубящий удар (6 кадров): замах - взмах - доводка."""
    bob = [0, 1, 0, 0, 0, 0][i]
    lean = [-1, -1, 1, 1, 0, 0][i]
    c, by = pl_base(bob=bob, cloak=[2, 3, 0, -1, 0, 1][i], legF=[0, -1, 2, 2, 1, 0][i], legB=[0, 1, -1, -1, 0, 0][i], lean=lean)
    ang = [205, 235, 330, 15, 65, 100][i]
    hx, hy = (19, 17) if i < 2 else (21, 17)
    pl_arm_sword(c, ang, by, hx=hx + lean, hy=hy, L=12)
    if i == 3:
        trail2(c, hx + lean, hy + by, 11, 300, 420, AMBER_HI, AMBER)
    if i == 4:
        trail2(c, hx + lean, hy + by, 11, 10, 70, AMBER, EMBER_D)
    return c


def pl_attack2(i):
    """Рубящий сверху (6 кадров)."""
    bob = [0, 1, 1, 0, 0, 0][i]
    lean = [-1, -2, 0, 2, 1, 0][i]
    c, by = pl_base(bob=bob, cloak=[2, 3, 1, -2, -1, 0][i], legF=[0, -1, 1, 3, 2, 0][i], legB=[0, 1, 0, -2, -1, 0][i], lean=lean)
    ang = [280, 300, 355, 80, 85, 100][i]
    hx, hy = 20 + lean, (14 if i < 2 else 16)
    pl_arm_sword(c, ang, by, hx=hx, hy=hy, L=12)
    if i == 3:
        trail2(c, hx, hy + by - 2, 12, 250, 380, AMBER_HI, AMBER)
        sparks(c, 25, 26, AMBER_HI, 5, 7)
        c.r(24, 27, 4, 1, EMBER_D)           # вырубина в земле
    if i == 4:
        sparks(c, 25, 26, AMBER, 3, 5)
    return c


def pl_cast(i):
    """Колдовство: угольная сфера в ладони (5 кадров)."""
    bob = [0, 0, -1, 0, 0][i]
    c, by = pl_base(bob=bob, cloak=[0, 1, 2, 3, 1][i], lean=[0, 0, -1, 1, 0][i])
    pl_arm_sword(c, 125, by)                   # меч опущен
    hx = [21, 22, 22, 25, 24][i]
    hy = [14, 12, 11, 13, 13][i]
    c.line(19, 15 + by, hx - 1, hy + by + 1, ARMOR, 2)   # свободная рука вверх
    c.px(hx, hy + by, SKIN_D)
    if i >= 1:
        rr = [0, 1, 2, 2, 1][i]
        c.ell(hx + 1, hy + by - 1, rr, rr, EMBER)
        c.px(hx + 1, hy + by - 1, AMBER_HI)
    if i == 2:
        for a in range(0, 360, 60):
            c.px(hx + 1 + math.cos(math.radians(a)) * 3.4, hy + by - 1 + math.sin(math.radians(a)) * 3.4, AMBER)
    if i == 3:
        trail2(c, hx + 1, hy + by - 1, 4, 200, 120, AMBER_HI, EMBER, 10)
        sparks(c, hx + 3, hy + by, EMBER, 4, 3)
    return c


def pl_shoot(i):
    """Выстрел из лука (4 кадра)."""
    bob = [0, 0, 0, 0][i]
    c, by = pl_base(bob=bob, cloak=[0, 1, 2, 1][i], legF=[1, 2, 2, 1][i], legB=[-1, -2, -1, 0][i])
    bx = [24, 25, 25, 24][i]
    bowv(c, bx, 14 + by, 13, pull=[1, 5, 0, 0][i])
    # задняя рука на тетиве
    pullx = [22, 19, 23, 23][i]
    c.line(14, 15 + by, pullx, 14 + by, ARMOR, 2)
    c.px(pullx, 14 + by, SKIN_D)
    if i < 2:                                   # стрела на тетиве
        c.line(pullx, 14 + by, bx + 1, 14 + by, WOOD, 1)
        c.px(bx + 1, 14 + by, BONE)
    if i == 2:                                  # вылет
        c.line(bx + 3, 14 + by, bx + 7, 14 + by, WOOD, 1)
        c.px(bx + 7, 14 + by, BONE)
    return c


def pl_roll(i):
    """Кувырок (4 кадра)."""
    if i == 0:
        c, by = pl_base(bob=2, cloak=2, legF=2, legB=-1, lean=1)
        pl_arm_sword(c, 60, by, hy=19)
        dustp(c, 10, 27, 3)
        return c
    if i in (1, 2):
        c = C()
        cy = 20 if i == 1 else 19
        cx = 15 + i * 2
        c.ell(cx, cy, 6, 5, EMBER_D)          # клубок плаща
        c.ell(cx, cy, 3, 3, ARMOR)            # свернувшийся доспех
        rot = [0, 120][i - 1]
        for a in range(0, 360, 72):
            aa = math.radians(a + rot)
            c.px(cx + math.cos(aa) * 4.6, cy + math.sin(aa) * 4.2, AMBER)
        c.px(cx + 1, cy - 2, ARMOR_L)
        c.px(cx - 8, cy - 1, ASH_D); c.px(cx - 10, cy + 1, ASH_DD)   # штрихи скорости
        c.px(cx - 9, cy + 3, ASH_D)
        dustp(c, cx - 6, 27, 3)
        return c
    c, by = pl_base(bob=2, cloak=1, legF=2, legB=0, lean=1)
    pl_arm_sword(c, 95, by, hy=19)
    c.line(19, 16 + by, 22, 24, ARMOR, 2)      # рука в землю
    dustp(c, 10, 27, 2)
    return c


def pl_death(i):
    """Смерть: пошатнулся - колено - падение - пепел (5 кадров)."""
    if i == 0:
        c, by = pl_base(bob=0, cloak=3, lean=-2)
        pl_arm_sword(c, 250, by, hx=19)
        return c
    if i == 1:
        c = C()
        c.r(13, 24, 2, 3, LEGS); c.r(17, 24, 2, 3, LEGS)
        c.r(12, 26, 3, 1, DARK); c.r(17, 26, 3, 1, DARK)
        c.poly([(12, 17), (19, 16), (20, 24), (12, 24)], ARMOR)   # торс ниже
        c.r(12, 23, 8, 1, DARK)
        c.poly([(12, 10), (19, 10), (20, 14), (19, 17), (12, 17), (11, 13)], ASH_D)
        c.r(16, 12, 3, 4, (28, 22, 32))
        eyes2(c, 16, 13, EMBER)
        c.poly([(10, 16), (8, 24), (12, 24)], EMBER_D)            # плащ свисает
        c.line(20, 18, 24, 26, ARMOR, 2)
        sword(c, 24, 26, 100, 8)
        return c
    if i == 2:
        c = C()
        c.poly([(8, 22), (22, 21), (24, 25), (9, 26)], ARMOR)     # тело лёжа
        c.poly([(7, 22), (12, 22), (12, 26), (7, 26)], EMBER_D)
        c.poly([(22, 20), (27, 20), (28, 24), (23, 25)], ASH_D)   # голова
        c.r(25, 22, 2, 2, (28, 22, 32))
        c.px(26, 22, EMBER)
        c.r(9, 26, 8, 1, LEGS)
        sword(c, 27, 26, 15, 8)
        return c
    if i == 3:
        c = C()
        c.poly([(9, 23), (18, 22), (19, 25), (10, 26)], ARMOR)    # тело истлевает
        c.poly([(8, 23), (11, 23), (11, 26), (8, 26)], EMBER_D)
        c.px(14, 23, INK2); c.px(16, 25, INK2); c.px(12, 25, INK2)
        c.poly([(21, 21), (25, 21), (25, 24), (21, 24)], ASH_D)
        for k, (sx, sy) in enumerate([(12, 18), (15, 15), (18, 19), (21, 14), (24, 17)]):
            c.px(sx, sy, ASH if k % 2 else EMBER)
        sword(c, 26, 26, 15, 7)
        return c
    c = C()
    c.poly([(10, 25), (22, 25), (20, 27), (12, 27)], ASH_DD)     # кучка пепла
    c.px(13, 24, ASH); c.px(18, 24, ASH)
    c.px(15, 21, EMBER); c.px(19, 17, AMBER); c.px(12, 15, EMBER_D)
    sword(c, 24, 26, 20, 7, blade=BONE_D)
    return c


def player_frames():
    fr = []
    fr += [pl_idle(i) for i in range(4)]
    fr += [pl_walk(i) for i in range(6)]
    fr += [pl_attack(i) for i in range(6)]
    fr += [pl_attack2(i) for i in range(6)]
    fr += [pl_cast(i) for i in range(5)]
    fr += [pl_shoot(i) for i in range(4)]
    fr += [pl_roll(i) for i in range(4)]
    fr += [pl_death(i) for i in range(5)]
    return fr


# ============================================================
#  e_scav - Пепельный падальщик (сутулый, череп-маска, тесак)
# ============================================================
def scav_body(bob=0, hunch=0, legF=0, legB=0, tilt=0):
    c = C()
    c.r(13 + legB, 21 + bob, 2, 5, DARK)
    c.r(13 + legB, 25 + bob, 2, 2, BONE_DD)
    c.r(17 + legF, 21 + bob, 2, 5, DARK)
    c.r(17 + legF, 25 + bob, 2, 2, BONE_DD)
    c.px(14 + legB, 23 + bob, BONE_D)         # бинты
    c.px(18 + legF, 22 + bob, BONE_D)
    # сутулый торс (наклон вперёд) + горб
    c.poly([(11 - hunch, 13 + bob + tilt), (19, 12 + bob), (21, 20 + bob), (12, 21 + bob)], CLOTH)
    c.poly([(11 - hunch, 13 + bob + tilt), (14, 13 + bob), (15, 21 + bob), (12, 21 + bob)], CLOTH_D)
    c.poly([(10 - hunch, 11 + bob + tilt), (15, 11 + bob + tilt), (16, 15 + bob), (11, 15 + bob)], CLOTH_D)
    c.px(16, 16 + bob, CLOTH_D); c.px(18, 18 + bob, CLOTH_D); c.px(14, 19 + bob, CLOTH_D)
    c.line(13, 14 + bob, 18, 19 + bob, BONE_DD, 1)   # перевязь
    # лохмотья
    c.px(12, 21 + bob, CLOTH_D); c.px(15, 22 + bob, CLOTH_D); c.px(19, 21 + bob, CLOTH_D)
    # череп-маска
    hx = 19 + hunch // 2
    c.r(hx, 7 + bob + tilt, 6, 5, BONE)
    c.r(hx, 12 + bob + tilt, 4, 2, BONE_D)    # челюсть
    c.px(hx + 1, 9 + bob + tilt, INK)         # глазницы
    c.px(hx + 3, 9 + bob + tilt, INK)
    c.px(hx + 2, 13 + bob + tilt, INK)        # зубы
    c.px(hx, 13 + bob + tilt, INK)
    return c, hx, bob


def scav_cleaver(c, hx, hy, ang, big=0):
    """Ржавый тесак: широкое лезвие с зазубринами."""
    a = math.radians(ang)
    ca, sa = math.cos(a), math.sin(a)
    px_, py_ = -sa, ca
    L = 6 + big
    W = 3 + big
    pts = [(hx + ca * 2 - px_ * 1, hy + sa * 2 - py_ * 1),
           (hx + ca * L - px_ * W, hy + sa * L - py_ * W),
           (hx + ca * (L + 1) - px_ * (W - 1), hy + sa * (L + 1) - py_ * (W - 1)),
           (hx + ca * (L + 1) + px_ * 1, hy + sa * (L + 1) + py_ * 1),
           (hx + ca * 2 + px_ * 1, hy + sa * 2 + py_ * 1)]
    c.poly(pts, RUST)
    # кромка и зазубрины
    for t in (0.45, 0.75, 1.0):
        c.px(hx + ca * (L + 1) * t - px_ * (W - 1 - t), hy + sa * (L + 1) * t - py_ * (W - 1 - t), RUST_D)
    c.px(hx + ca * L - px_ * W, hy + sa * L - py_ * W, RUST_D)
    c.line(hx - ca * 2, hy - sa * 2, hx + ca * 2, hy + sa * 2, WOOD_D, 2)   # рукоять


def scav(i_anim, i):
    if i_anim == "idle":
        bob = [0, -1, -1, 0][i]
        c, hx, by = scav_body(bob=bob, hunch=1)
        c.line(19, 15 + by, 22, 18 + by, CLOTH_D, 2)          # рука
        scav_cleaver(c, 22, 18 + by, 95)
        if i == 3:
            c.px(hx + 1, 9 + by, EMBER); c.px(hx + 3, 9 + by, EMBER)
        return c
    if i_anim == "walk":
        legF = [2, 1, 0, -2][i]; legB = [-2, -1, 0, 2][i]
        bob = [0, -1, 0, -1][i]
        c, hx, by = scav_body(bob=bob, hunch=1, legF=legF, legB=legB)
        c.line(19, 15 + by, 22, 17 + by, CLOTH_D, 2)
        scav_cleaver(c, 22, 17 + by, 100)
        dustp(c, 12 - legF, 27, 2)
        return c
    if i_anim == "attack":
        poses = [(0, 250, 0), (1, 275, 0), (0, 20, 1), (0, 85, 1)]
        bob, ang, lean = poses[i]
        c, hx, by = scav_body(bob=bob, hunch=1 - lean, legF=lean * 2, tilt=-lean)
        a = math.radians(ang)
        hx2, hy2 = 20 + math.cos(a) * 2, 15 + by + math.sin(a) * 2
        c.line(18, 15 + by, hx2, hy2, CLOTH_D, 2)
        scav_cleaver(c, hx2, hy2, ang, big=1)
        if i == 2:
            trail2(c, 20, 15 + by, 8, 250, 400, RUST, RUST_D, 14)
        if i == 3:
            sparks(c, 25, 25, RUST, 4, 6)
        return c
    # death
    if i == 0:
        c, hx, by = scav_body(bob=0, hunch=0, tilt=-1)
        c.line(18, 14, 15, 9, CLOTH_D, 2)
        return c
    if i == 1:
        c = C()
        c.r(13, 23, 2, 3, DARK); c.r(17, 23, 2, 3, DARK)
        c.poly([(12, 17), (19, 16), (20, 23), (12, 23)], CLOTH)
        c.r(18, 12, 5, 4, BONE)
        c.px(19, 13, INK); c.px(21, 13, INK)
        return c
    if i == 2:
        c = C()
        c.poly([(9, 23), (20, 22), (21, 26), (10, 26)], CLOTH)
        c.px(12, 24, CLOTH_D); c.px(16, 23, CLOTH_D)
        c.r(23, 22, 5, 4, BONE)               # череп откатился
        c.px(24, 23, INK); c.px(26, 23, INK)
        c.r(23, 26, 4, 1, BONE_D)
        return c
    c = C()
    c.poly([(11, 25), (21, 25), (19, 27), (13, 27)], CLOTH_D)
    c.r(23, 23, 5, 4, BONE_D)                 # череп
    c.px(24, 24, INK); c.px(26, 24, INK)
    c.px(15, 22, ASH); c.px(18, 20, ASH); c.px(13, 19, EMBER_D)
    return c


def scav_frames():
    return ([scav("idle", i) for i in range(4)] + [scav("walk", i) for i in range(4)] +
            [scav("attack", i) for i in range(4)] + [scav("death", i) for i in range(4)])


# ============================================================
#  e_arch - Костяной лучник (скелет с луком)
# ============================================================
def skel_head(c, x, y, glow=False):
    c.r(x, y, 6, 5, BONE)
    c.r(x + 1, y + 5, 4, 1, BONE_D)
    c.px(x + 1, y + 2, INK); c.px(x + 3, y + 2, INK)
    if glow:
        c.px(x + 1, y + 2, EMBER); c.px(x + 3, y + 2, EMBER)
    c.px(x + 2, y + 4, BONE_D)
    c.px(x + 1, y + 5, INK); c.px(x + 3, y + 5, INK)


def arch_body(bob=0, legF=0, legB=0):
    c = C()
    c.r(13 + legB, 21 + bob, 2, 5, BONE_D)
    c.r(13 + legB, 26 + bob, 3, 1, BONE_DD)
    c.r(17 + legF, 21 + bob, 2, 5, BONE_D)
    c.r(17 + legF, 26 + bob, 3, 1, BONE_DD)
    c.r(15, 20 + bob, 5, 2, BONE_D)           # таз
    c.r(16, 13 + bob, 2, 7, BONE)             # позвоночник
    for ry in (14, 16, 18):                   # рёбра
        c.r(13, ry + bob, 8, 1, BONE)
        c.px(14, ry + bob, BONE_D)
    skel_head(c, 15, 6 + bob)
    return c


def arch_frames_f():
    fr = []
    for i in range(4):                        # idle
        bob = [0, -1, -1, 0][i]
        c = arch_body(bob=bob)
        c.line(18, 14 + bob, 22, 15 + bob, BONE_D, 2)         # рука к луку
        bowv(c, 24, 14 + bob, 12)
        # колчан
        c.r(10, 12 + bob, 3, 7, WOOD_D)
        c.px(10, 10 + bob, BONE); c.px(12, 10 + bob, BONE)
        if i == 2:
            skel_head(c, 15, 6 + bob, glow=True)
            c.r(16, 13 + bob, 2, 7, BONE)
            for ry in (14, 16, 18):
                c.r(13, ry + bob, 8, 1, BONE)
        fr.append(c)
    for i in range(4):                        # walk
        legF = [2, 1, -1, -2][i]; legB = [-2, -1, 1, 2][i]
        bob = [0, -1, 0, -1][i]
        c = arch_body(bob=bob, legF=legF, legB=legB)
        c.line(18, 14 + bob, 22, 15 + bob, BONE_D, 2)
        bowv(c, 24, 14 + bob, 12)
        c.r(10, 12 + bob, 3, 7, WOOD_D)
        dustp(c, 12, 27, 2)
        fr.append(c)
    for i in range(4):                        # attack: натянул - выстрел - ...
        bob = [0, 0, 0, -1][i]
        c = arch_body(bob=bob, legF=[1, 2, 2, 1][i], legB=[-1, -2, -2, -1][i])
        pull = [2, 6, 0, 0][i]
        bx = [24, 25, 25, 24][i]
        bowv(c, bx, 14 + bob, 12, pull=pull)
        c.line(18, 14 + bob, bx - 2, 14 + bob, BONE_D, 2)
        c.line(13, 14 + bob, bx - pull - 1, 14 + bob, BONE_D, 1)   # задняя рука на тетиве
        c.r(10, 12 + bob, 3, 7, WOOD_D)
        if i < 2:
            c.line(bx - pull, 14 + bob, bx + 2, 14 + bob, WOOD, 1)
            c.px(bx + 2, 14 + bob, BONE)
        if i == 2:
            c.line(bx + 3, 14 + bob, bx + 7, 14 + bob, WOOD, 1)
            c.px(bx + 7, 14 + bob, AMBER_HI)
            c.px(10, 10 + bob, BONE)
        fr.append(c)
    for i in range(4):                        # death: кости рассыпались
        if i == 0:
            c = arch_body(bob=0, tilt=0) if False else arch_body(bob=1)
            c.line(13, 14, 9, 10, BONE_D, 2)
        elif i == 1:
            c = C()
            c.r(11, 22, 8, 3, BONE_D)
            skel_head(c, 19, 20)
            c.r(13, 25, 2, 2, BONE_DD)
        elif i == 2:
            c = C()
            c.r(10, 25, 7, 2, BONE_D)         # груда рёбер
            c.px(11, 24, BONE); c.px(13, 24, BONE); c.px(15, 24, BONE)
            skel_head(c, 20, 23)
            c.r(25, 20, 2, 6, BONE_D)         # бедро торчит
            c.r(7, 22, 2, 5, BONE_DD)
        else:
            c = C()
            c.r(10, 26, 7, 1, BONE_DD)
            c.r(11, 25, 5, 1, BONE_D)
            skel_head(c, 21, 24)
            c.r(26, 22, 2, 5, BONE_DD)
            c.px(15, 21, ASH); c.px(19, 19, ASH)
        fr.append(c)
    return fr


# ============================================================
#  e_beetle - Угольный жук (тлеющие трещины панциря)
# ============================================================
def beetle(i_anim, i):
    c = C()
    glow = [EMBER, EMBER_HI, EMBER, EMBER_D]
    g = glow[i] if i_anim in ("idle", "walk") else EMBER
    bx, by = 15, 18
    if i_anim == "attack":
        bx += [0, -1, 3, 1][i]
        g = [EMBER, AMBER_HI, AMBER_HI, EMBER][i]
    if i_anim == "death":
        g = [AMBER_HI, AMBER_HI, EMBER, EMBER_D][i]
    # ноги
    step = [0, 1, 0, -1][i] if i_anim == "walk" else 0
    for k, lx in enumerate((9, 14, 19)):
        off = step if k % 2 == 0 else -step
        c.line(lx + off, by + 3, lx - 2 + off, 26, DARK, 1)
        c.line(lx + off + 4, by + 3, lx + 6 + off, 26, DARK, 1)
    # тело
    c.ell(bx, by, 8, 5, (34, 30, 38))
    c.ell(bx, by - 1, 7, 4, (48, 42, 52))
    # сегменты + тлеющие трещины
    for sx in (bx - 4, bx, bx + 4):
        c.line(sx, by - 4, sx, by + 3, INK, 1)
        for yy in range(by - 3, by + 3, 2):
            c.px(sx + (1 if yy % 4 == 1 else -0), yy, g)
    c.px(bx - 2, by - 2, g); c.px(bx + 2, by, g); c.px(bx, by + 2, g)
    if i_anim == "death" and i >= 1:
        c.line(bx, by - 5, bx, by + 4, AMBER_HI, 1)      # трещина сквозь панцирь
    if i_anim == "death" and i == 2:
        c.line(bx - 1, by - 5, bx - 1, by + 4, INK, 2)
        sparks(c, bx, by - 3, AMBER_HI, 6, 11)
    if i_anim == "death" and i == 3:
        c.ell(bx, by + 1, 6, 3, (30, 26, 32))
        c.px(bx - 2, by, EMBER_D); c.px(bx + 2, by + 1, EMBER_D)
        c.px(bx - 4, by - 3, ASH); c.px(bx + 4, by - 4, ASH)
    # голова
    hx = bx + 8
    c.r(hx - 1, by - 2, 4, 4, (40, 34, 42))
    c.px(hx + 1, by - 1, AMBER); c.px(hx + 1, by + 1, AMBER)   # глаза
    mand = 2 if (i_anim == "attack" and i in (1, 2)) else 0
    c.line(hx + 3, by - 2, hx + 5, by - 3 - mand, DARK, 1)
    c.line(hx + 3, by + 1, hx + 5, by + 2 + mand, DARK, 1)
    if i_anim == "attack" and i == 2:
        dustp(c, bx - 8, 26, 3)
        c.px(hx + 6, by - 4, EMBER); c.px(hx + 6, by + 3, EMBER)
    if i_anim == "death" and i == 3:
        return c
    return c


def beetle_frames():
    return ([beetle("idle", i) for i in range(4)] + [beetle("walk", i) for i in range(4)] +
            [beetle("attack", i) for i in range(4)] + [beetle("death", i) for i in range(4)])


# ============================================================
#  e_axer - Топорщик пустоши (громоздкий, огромный топор)
# ============================================================
def axer_body(bob=0, legF=0, legB=0, lean=0):
    c = C()
    c.r(12 + legB, 21 + bob, 3, 5, (66, 60, 74))
    c.r(11 + legB, 25 + bob, 4, 2, DARK)
    c.r(18 + legF, 21 + bob, 3, 5, (66, 60, 74))
    c.r(18 + legF, 25 + bob, 4, 2, DARK)
    c.px(13 + legB, 23 + bob, STEEL_D); c.px(19 + legF, 23 + bob, STEEL_D)   # наколенники
    # торс: сужение к поясу, бока в тени
    c.poly([(11 + lean, 11 + bob), (22 + lean, 11 + bob), (23, 17 + bob),
            (21, 21 + bob), (12, 21 + bob), (10, 17 + bob)], ARMOR)
    c.poly([(11 + lean, 11 + bob), (14 + lean, 11 + bob), (13, 21 + bob), (12, 21 + bob), (10, 17 + bob)], ASH_DD)
    c.r(14 + lean, 12 + bob, 3, 7, ARMOR_L)                    # нагрудная пластина
    c.px(10, 17 + bob, ASH_DD); c.px(23, 17 + bob, ASH_DD)
    c.r(11, 20 + bob, 11, 1, DARK)
    c.px(16 + lean, 20 + bob, AMBER)
    # наплечники-обломки
    c.poly([(8 + lean, 11 + bob), (12 + lean, 10 + bob), (12 + lean, 14 + bob), (8 + lean, 13 + bob)], STEEL_D)
    c.poly([(21 + lean, 10 + bob), (25 + lean, 11 + bob), (25 + lean, 13 + bob), (21 + lean, 14 + bob)], STEEL_D)
    c.px(9 + lean, 12 + bob, STEEL); c.px(23 + lean, 12 + bob, STEEL)   # заклёпки
    # шлем
    c.r(13 + lean, 4 + bob, 7, 6, STEEL_D)
    c.r(14 + lean, 5 + bob, 5, 2, STEEL)
    c.r(14 + lean, 7 + bob, 6, 1, INK)        # прорезь
    c.px(15 + lean, 7 + bob, EMBER); c.px(18 + lean, 7 + bob, EMBER)
    return c


def axer(i_anim, i):
    if i_anim == "idle":
        bob = [0, 0, -1, -1][i]
        c = axer_body(bob=bob)
        axe(c, 24, 20 + bob, 285, 12)
        c.line(21, 14 + bob, 24, 19 + bob, ARMOR, 2)
        if i == 3:
            c.px(15, 7 + bob, AMBER_HI); c.px(18, 7 + bob, AMBER_HI)
            c.r(14 + 0, 7 + bob, 6, 1, INK)
            c.px(15, 7 + bob, AMBER_HI); c.px(18, 7 + bob, AMBER_HI)
        return c
    if i_anim == "walk":
        legF = [2, 1, -1, -2][i]; legB = [-2, -1, 1, 2][i]
        bob = [0, 1, 0, 1][i]
        c = axer_body(bob=bob, legF=legF, legB=legB)
        axe(c, 24, 21 + bob, 280, 12)
        c.line(21, 15 + bob, 24, 20 + bob, ARMOR, 2)
        dustp(c, 11 - legF, 27, 3)
        return c
    if i_anim == "attack":
        cfg = [(0, 245, -1), (1, 262, -2), (0, 25, 2), (1, 78, 2)]
        bob, ang, lean = cfg[i]
        c = axer_body(bob=bob, lean=lean, legF=lean * 2)
        hx, hy = 22 + lean, (13 if i < 2 else 18)
        axe(c, hx, hy + bob, ang, 13)
        c.line(20, 14 + bob, hx - 1, hy + bob + 1, ARMOR, 2)
        if i == 2:
            trail2(c, 20, 14 + bob, 12, 285, 405, AMBER_HI, AMBER)
        if i == 3:
            sparks(c, 26, 25, AMBER_HI, 6, 9)
            c.r(23, 27, 6, 1, EMBER_D)
            dustp(c, 20, 27, 3)
        return c
    # death
    if i == 0:
        c = axer_body(bob=0, lean=-2)
        axe(c, 22, 22, 60, 12)
        return c
    if i == 1:
        c = C()
        c.r(13, 23, 3, 3, LEGS); c.r(18, 23, 3, 3, LEGS)
        c.poly([(12, 15), (22, 15), (23, 23), (11, 23)], ARMOR)
        c.r(14, 10, 6, 5, STEEL_D)
        c.r(15, 12, 5, 1, INK)
        c.px(16, 12, EMBER_D)
        axe(c, 25, 24, 80, 10)
        return c
    if i == 2:
        c = C()
        c.poly([(8, 21), (22, 20), (24, 25), (9, 26)], ARMOR)
        c.r(9, 24, 8, 2, LEGS)
        c.r(23, 19, 6, 5, STEEL_D)
        c.r(24, 21, 4, 1, INK)
        axe(c, 14, 26, 10, 10)
        return c
    c = C()
    c.poly([(9, 24), (21, 23), (22, 26), (10, 27)], ARMOR_L)
    c.r(23, 22, 6, 4, STEEL_D)
    c.r(24, 24, 4, 1, INK)
    axe(c, 13, 26, 10, 9)
    c.px(14, 20, ASH); c.px(18, 18, ASH); c.px(21, 16, EMBER_D)
    return c


def axer_frames():
    return ([axer("idle", i) for i in range(4)] + [axer("walk", i) for i in range(4)] +
            [axer("attack", i) for i in range(4)] + [axer("death", i) for i in range(4)])


# ============================================================
#  e_dustm - Праховый маг (парит, посох с углём-кристаллом)
# ============================================================
def dustm(i_anim, i):
    c = C()
    bob = [0, -1, -2, -1][i] if i_anim in ("idle", "walk") else [0, -1, -2, -1][i]
    flare = False
    hx, hy = 16, 8 + bob                      # капюшон
    if i_anim == "death":
        bob = [0, 1, 3, 4][i]
        hy = 8 + bob
    # роба (парящая, подол распадается в прах)
    c.poly([(12, 13 + bob), (20, 13 + bob), (22, 24 + bob), (10, 24 + bob)], DUST)
    c.poly([(12, 13 + bob), (15, 13 + bob), (14, 24 + bob), (10, 24 + bob)], DUST_D)
    for k in range(0, 10, 2):                 # кайма пунктиром
        c.px(11 + k, 23 + bob, AMBER)
    # прах вместо ног: редкие пылинки, дрейф вниз-вбок
    sp = [(10, 25), (14, 26), (17, 25), (21, 26), (12, 27), (19, 27)]
    for k, (sx, sy) in enumerate(sp):
        if (i + k) % 3 == 2:
            continue
        c.px(sx + (i + k) % 2, sy + bob, BONE_DD if k % 2 else DUST_D)
    c.px(13 + (i % 3), 21 - i % 2, ASH_D)     # восходящие пылинки
    c.px(19 - (i % 2), 20 - (i + 1) % 2, BONE_DD)
    if i_anim == "death":
        if i == 1:
            c.poly([(12, 13 + bob), (20, 13 + bob), (21, 22 + bob), (11, 22 + bob)], DUST)
        if i == 2:
            c.im = Image.new("RGBA", (LOG, LOG), (0, 0, 0, 0)); c.d = ImageDraw.Draw(c.im)
            c.poly([(10, 22), (22, 22), (23, 26), (9, 26)], DUST_D)
            c.px(12, 23, DUST); c.px(18, 24, DUST)
            c.r(9, 25, 14, 1, AMBER)
            c.line(24, 12, 26, 26, WOOD_D, 1)          # упавший посох
            c.px(24, 11, EMBER_D)
            c.px(14, 18, ASH); c.px(18, 15, ASH)
            return c
        if i == 3:
            c.im = Image.new("RGBA", (LOG, LOG), (0, 0, 0, 0)); c.d = ImageDraw.Draw(c.im)
            c.poly([(11, 24), (21, 24), (22, 27), (10, 27)], DUST_D)
            c.px(13, 25, DUST)
            c.line(24, 16, 26, 26, WOOD_D, 1)
            c.px(24, 15, EMBER_DD)
            c.px(13, 20, ASH); c.px(16, 16, ASH); c.px(19, 12, EMBER_D); c.px(11, 14, ASH)
            return c
    # капюшон
    c.poly([(hx - 4, hy + 1), (hx + 4, hy + 1), (hx + 5, hy + 5), (hx + 3, hy + 7),
            (hx - 3, hy + 7), (hx - 5, hy + 5)], DUST_D)
    c.r(hx - 2, hy + 2, 5, 4, (24, 20, 26))
    eyes2(c, hx - 1, hy + 3, AMBER_HI if not (i_anim == "death") else EMBER_D)
    # посох
    sx, sy = 24, 10 + bob
    if i_anim == "attack":
        sy = [8, 5, 12, 12][i] + bob
        sx = [24, 25, 27, 26][i]
        flare = i in (1, 2)
    c.line(sx, sy, sx - 1, 24 + bob, WOOD_D, 1)
    c.px(sx - 1, 24 + bob, WOOD)
    # кристалл-уголёк (парит над посохом)
    cy = sy - 3 + (1 if i % 2 else 0)
    if i_anim == "death":
        cy = sy - 1
    rr = 2 if not flare else 3
    c.poly([(sx, cy - rr), (sx + rr, cy), (sx, cy + rr), (sx - rr, cy)], EMBER)
    c.px(sx, cy, AMBER_HI)
    c.px(sx, cy - 1, AMBER)
    if flare:
        for a in range(0, 360, 45):
            c.px(sx + math.cos(math.radians(a)) * (rr + 2), cy + math.sin(math.radians(a)) * (rr + 2), AMBER)
    if i_anim == "attack" and i == 2:
        c.line(sx + 3, cy, sx + 7, cy, EMBER, 1)
        c.px(sx + 7, cy, AMBER_HI)
        c.px(sx + 5, cy - 1, EMBER_D); c.px(sx + 6, cy + 1, EMBER_D)
    # рука-рукав к посоху
    c.line(19, 15 + bob, sx - 1, sy + 4, DUST, 2)
    c.px(sx - 1, sy + 4, AMBER)
    return c


def dustm_frames():
    return ([dustm("idle", i) for i in range(4)] + [dustm("walk", i) for i in range(4)] +
            [dustm("attack", i) for i in range(4)] + [dustm("death", i) for i in range(4)])


# ============================================================
#  NPC: npc0 торговец, npc1 кузнец, npc2 целительница, npc3 старейшина
# ============================================================
def npc_robe(c, col, cold, hem=26, bob=0, trim=None):
    c.poly([(12, 13 + bob), (20, 13 + bob), (22, hem + bob), (10, hem + bob)], col)
    c.poly([(12, 13 + bob), (15, 13 + bob), (14, hem + bob), (10, hem + bob)], cold)
    if trim:
        for x in range(10, 22, 2):
            c.px(x, hem - 1 + bob, trim)


def npc0_frames():                            # торговец Грим
    fr = []
    for i in range(4):
        c = C()
        bob = [0, -1, -1, 0][i]
        npc_robe(c, ROBE_P, ROBE_PD, bob=bob, trim=AMBER)
        # капюшон за головой + лицо
        c.poly([(12, 5 + bob), (20, 5 + bob), (21, 11 + bob), (19, 13 + bob), (13, 13 + bob), (11, 11 + bob)], ROBE_PD)
        c.r(14, 7 + bob, 5, 5, SKIN)
        c.px(15, 9 + bob, INK); c.px(17, 9 + bob, INK)
        c.px(16, 11 + bob, SKIN_D)
        c.r(14, 6 + bob, 5, 1, INK2)          # тень капюшона
        # кошель
        c.r(19, 18 + bob, 3, 3, APRON_D)
        c.px(20, 19 + bob, AMBER if i != 2 else AMBER_HI)
        # руки (тёмнее робы, чтобы читались)
        if i == 3:
            c.line(13, 15 + bob, 11, 12 + bob, ROBE_PD, 2)  # жест
            c.px(11, 11 + bob, SKIN); c.px(11, 12 + bob, SKIN)
            c.line(19, 15 + bob, 20, 17 + bob, ROBE_PD, 2)
            c.px(20, 17 + bob, SKIN)
        else:
            c.line(13, 15 + bob, 15, 18 + bob, ROBE_PD, 2)
            c.line(19, 15 + bob, 17, 18 + bob, ROBE_PD, 2)
            c.px(15, 18 + bob, SKIN); c.px(17, 18 + bob, SKIN)
        fr.append(c)
    return fr


def npc1_frames():                            # кузнец Уголь
    fr = []
    for i in range(4):
        c = C()
        bob = [0, 0, -1, -1][i]
        # ноги
        c.r(13, 22 + bob, 3, 4, LEGS); c.r(18, 22 + bob, 3, 4, LEGS)
        c.r(12, 26 + bob, 4, 1, DARK); c.r(18, 26 + bob, 4, 1, DARK)
        # торс + фартук
        c.r(11, 12 + bob, 10, 10, SKIN_D)
        c.poly([(12, 13 + bob), (20, 13 + bob), (21, 23 + bob), (11, 23 + bob)], APRON)
        c.px(14, 17 + bob, INK2); c.px(18, 19 + bob, INK2); c.px(16, 21 + bob, INK2)  # сажа
        c.r(11, 12 + bob, 10, 1, APRON_D)
        # голова: лысина + борода
        c.r(13, 5 + bob, 6, 6, SKIN)
        c.px(14, 7 + bob, INK); c.px(17, 7 + bob, INK)
        c.poly([(13, 9 + bob), (19, 9 + bob), (18, 13 + bob), (14, 13 + bob)], GREY_BEARD)
        c.px(15, 11 + bob, WHITE)
        c.px(13, 4 + bob, SKIN_D); c.px(18, 4 + bob, SKIN_D)
        # молот на плече
        hx2 = 22 if i < 2 else 21
        c.line(hx2, 16 + bob, hx2 + 1, 7 + bob, WOOD_D, 2)
        c.r(hx2 - 1, 4 + bob, 5, 3, STEEL_D)
        c.r(hx2, 5 + bob, 3, 1, STEEL)
        c.line(19, 14 + bob, hx2, 15 + bob, SKIN_D, 2)
        c.line(11, 14 + bob, 10, 18 + bob, SKIN_D, 2)
        if i == 3:
            c.px(hx2 + 3, 5 + bob, AMBER_HI)   # блик
        fr.append(c)
    return fr


def npc2_frames():                            # целительница Ива
    fr = []
    for i in range(4):
        c = C()
        bob = [0, -1, 0, -1][i]
        npc_robe(c, ROBE_G, ROBE_GD, bob=bob, trim=BONE)
        # белый плат/капюшон
        c.poly([(12, 5 + bob), (20, 5 + bob), (21, 12 + bob), (18, 13 + bob), (14, 13 + bob), (11, 12 + bob)], WHITE)
        c.r(14, 7 + bob, 5, 5, SKIN)
        c.px(15, 9 + bob, INK); c.px(17, 9 + bob, INK)
        c.px(16, 11 + bob, (220, 140, 130))
        c.poly([(13, 6 + bob), (19, 6 + bob), (19, 7 + bob), (13, 7 + bob)], ROBE_GD)
        # фонарь
        lx = 23 + (1 if i % 2 else 0)
        c.line(19, 15 + bob, lx - 1, 14 + bob, ROBE_GD, 2)
        c.line(lx, 12 + bob, lx, 14 + bob, DARK, 1)
        c.r(lx - 1, 14 + bob, 3, 4, DARK)
        glow = AMBER_HI if i in (1, 2) else AMBER
        c.px(lx, 15 + bob, glow); c.px(lx, 16 + bob, glow)
        halo = (255, 200, 120, 110) if i in (1, 2) else (255, 200, 120, 70)
        c.px(lx - 2, 15 + bob, halo); c.px(lx + 2, 15 + bob, halo)
        c.px(lx, 13 + bob, halo); c.px(lx - 1, 18 + bob, halo)
        c.line(13, 15 + bob, 12, 18 + bob, ROBE_GD, 2)
        c.px(12, 18 + bob, SKIN)
        fr.append(c)
    return fr


def npc3_frames():                            # старейшина
    fr = []
    for i in range(4):
        c = C()
        bob = [0, 0, -1, -1][i]
        npc_robe(c, ROBE_B, ROBE_BD, bob=bob, trim=AMBER)
        # сутулость: плечо вперёд
        c.poly([(12, 12 + bob), (20, 13 + bob), (20, 16 + bob), (12, 15 + bob)], ROBE_BD)
        # капюшон + длинная борода
        c.poly([(12, 5 + bob), (20, 5 + bob), (21, 11 + bob), (19, 13 + bob), (13, 13 + bob), (11, 11 + bob)], ROBE_BD)
        c.r(14, 7 + bob, 5, 4, SKIN_D)
        c.px(15, 9 + bob, INK); c.px(17, 9 + bob, INK)
        c.poly([(14, 11 + bob), (19, 11 + bob), (18, 18 + bob), (15, 18 + bob)], WHITE)
        c.px(16, 13 + bob, GREY_BEARD); c.px(15, 15 + bob, GREY_BEARD)
        # посох
        c.line(24, 8 + bob, 23, 26 + bob, WOOD, 1)
        c.px(24, 7 + bob, WOOD_D); c.px(25, 8 + bob, WOOD_D)
        c.px(24, 6 + bob, AMBER if i < 2 else AMBER_HI)
        c.line(19, 15 + bob, 23, 16 + bob, ROBE_BD, 2)
        c.px(23, 16 + bob, SKIN_D)
        fr.append(c)
    return fr


# ============================================================
#  сборка листов
# ============================================================
def build_sheet(frames, cols, key, fps, anims, ax, ay, outdir, do_outline=True):
    rows = (len(frames) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * OUT, rows * OUT), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        if do_outline:
            outline(f)
        big = f.im.resize((OUT, OUT), Image.NEAREST)
        sheet.paste(big, ((i % cols) * OUT, (i // cols) * OUT), big)
    png = os.path.join(outdir, key + ".png")
    sheet.save(png)
    meta = {"key": key, "frameW": OUT, "frameH": OUT,
            "anchorX": ax, "anchorY": ay, "fps": fps, "anims": anims}
    with open(os.path.join(outdir, key + ".json"), "w", encoding="utf-8") as fh:
        json.dump(meta, fh, ensure_ascii=False, indent=1)
    print("%-10s %2d кадров -> %s" % (key, len(frames), png))
    return sheet


MOB_ANIMS = {"idle": [0, 3], "walk": [4, 7], "attack": [8, 11], "death": [12, 15]}
PLAYER_ANIMS = {"idle": [0, 3], "walk": [4, 9], "attack": [10, 15], "attack2": [16, 21],
                "cast": [22, 26], "shoot": [27, 30], "roll": [31, 34], "death": [35, 39]}
NPC_ANIMS = {"idle": [0, 3]}


def make_preview(sheets, path):
    """Контактный лист для визуальной проверки."""
    pad, scale = 4, 3
    rows = []
    for key, (frames, cols) in sheets.items():
        w = len(frames) * (LOG * scale + pad) + pad
        rows.append((key, frames, w))
    H = sum(LOG * scale + pad + 12 for _, _, _ in rows) + pad
    W = max(w for _, _, w in rows)
    out = Image.new("RGBA", (W, H), (20, 17, 28, 255))
    d = ImageDraw.Draw(out)
    y = pad
    for key, frames, _ in rows:
        d.text((pad, y), key, fill=(242, 181, 68, 255))
        y += 12
        for i, f in enumerate(frames):
            big = f.im.resize((LOG * scale, LOG * scale), Image.NEAREST)
            out.paste(big, (pad + i * (LOG * scale + pad), y), big)
            if i % 4 == 0:
                d.rectangle([pad + i * (LOG * scale + pad), y + LOG * scale + 1,
                             pad + i * (LOG * scale + pad) + 6, y + LOG * scale + 3],
                            fill=(255, 122, 46, 255))
        y += LOG * scale + pad
    out.save(path)
    print("превью ->", path)


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "assets", "sprites")
    os.makedirs(outdir, exist_ok=True)
    prevdir = os.path.join(root, "tools", "preview")
    os.makedirs(prevdir, exist_ok=True)

    sheets = {}

    pf = player_frames()
    sheets["player"] = (pf, 8)
    build_sheet(pf, 8, "player", 12, PLAYER_ANIMS, 32, 54, outdir)

    mobs = {
        "e_scav": (scav_frames(), 32, 54),
        "e_arch": (arch_frames_f(), 32, 54),
        "e_beetle": (beetle_frames(), 32, 52),
        "e_axer": (axer_frames(), 32, 54),
        "e_dustm": (dustm_frames(), 32, 56),
    }
    for key, (fr, ax, ay) in mobs.items():
        sheets[key] = (fr, 4)
        build_sheet(fr, 4, key, 9, MOB_ANIMS, ax, ay, outdir)

    npcs = {"npc0": npc0_frames(), "npc1": npc1_frames(),
            "npc2": npc2_frames(), "npc3": npc3_frames()}
    for key, fr in npcs.items():
        sheets[key] = (fr, 2)
        build_sheet(fr, 2, key, 3, NPC_ANIMS, 32, 54, outdir)

    make_preview(sheets, os.path.join(prevdir, "all.png"))


if __name__ == "__main__":
    main()
