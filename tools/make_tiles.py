#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор оригинальных ТАЙЛСЕТОВ «Пепла и Янтаря».

Как и tools/make_sprites.py, рисует каждый пиксель программно в палитре игры
(biomePalette из src/World/Dungeon.cpp), поэтому тайлы гарантированно
сочетаются с процедурной графикой и спрайт-листами. Никаких нейросетей и
чужих ассетов.

Логическая ячейка 20x20, в файл пишется 40x40 (x2, nearest) - ровно размер
тайла карты (Dungeon::tileSize = 40), поэтому при запекании земли спрайты
ложатся пиксель-в-пиксель без масштабирования.

На биом - один атлас «tiles_<биом>» (23 ячейки, сетка 7x4):
  floor0..floor4    варианты пола            (5)
  road0..road2      дорога/тропа/булыжник    (3)
  wall0..wall2      кладка стены             (3)
  walltop0..1       верхняя грань стены      (2)
  hazard0..1        опасная зона             (2)
  water0..1         вода                     (2)
  deco0..deco5      мелкие детали пола (альфа-канал) (6)

Запуск:  python3 tools/make_tiles.py [outdir]
Результат: assets/tiles/tiles_<биом>.png + .json, tools/preview/tiles_*.png
"""
import json
import math
import os
import random
import sys
import zlib

from PIL import Image, ImageDraw

LOG = 20    # логический размер ячейки
OUT = 40    # размер ячейки в файле (x2)
COLS = 7    # ячеек в строке атласа

# порядок ячеек в атласе (строка за строкой)
NAMES = (["floor%d" % i for i in range(5)] +
         ["road%d" % i for i in range(3)] +
         ["wall%d" % i for i in range(3)] +
         ["walltop%d" % i for i in range(2)] +
         ["hazard%d" % i for i in range(2)] +
         ["water%d" % i for i in range(2)] +
         ["deco%d" % i for i in range(5 + 1)])

# ---------------- палитры биомов (из biomePalette, src/World/Dungeon.cpp) ----------------
BIOMES = {
    "ash": dict(
        floor1=(58, 50, 48), floor2=(46, 40, 40), floor3=(72, 62, 58),
        wall=(38, 32, 32), wallTop=(96, 82, 74), wallDark=(20, 17, 18),
        hazard=(255, 120, 40), accent=(226, 138, 60),
        water=(38, 66, 78), road=(66, 52, 44), speck="ash"),
    "ember": dict(
        floor1=(44, 52, 36), floor2=(34, 42, 30), floor3=(58, 66, 44),
        wall=(30, 34, 24), wallTop=(76, 92, 56), wallDark=(16, 20, 14),
        hazard=(255, 110, 40), accent=(240, 150, 60),
        water=(36, 60, 66), road=(62, 48, 34), speck="ember"),
    "frost": dict(
        floor1=(150, 172, 196), floor2=(124, 148, 176), floor3=(186, 204, 222),
        wall=(96, 116, 146), wallTop=(214, 230, 244), wallDark=(40, 52, 72),
        hazard=(120, 200, 255), accent=(150, 220, 255),
        water=(70, 110, 140), road=(116, 132, 152), speck="frost"),
    "crystal": dict(
        floor1=(46, 40, 66), floor2=(34, 30, 52), floor3=(62, 54, 88),
        wall=(32, 26, 50), wallTop=(120, 96, 190), wallDark=(16, 13, 28),
        hazard=(190, 120, 255), accent=(176, 132, 255),
        water=(40, 34, 72), road=(58, 46, 74), speck="crystal"),
    "swamp": dict(
        floor1=(48, 54, 38), floor2=(38, 44, 32), floor3=(62, 68, 46),
        wall=(30, 34, 26), wallTop=(84, 96, 62), wallDark=(14, 18, 14),
        hazard=(120, 200, 70), accent=(150, 220, 90),
        water=(34, 54, 44), road=(56, 48, 32), speck="swamp"),
    "ruins": dict(
        floor1=(96, 88, 74), floor2=(78, 72, 60), floor3=(116, 106, 88),
        wall=(66, 60, 50), wallTop=(150, 138, 116), wallDark=(28, 25, 22),
        hazard=(110, 160, 170), accent=(220, 190, 120),
        water=(52, 74, 80), road=(92, 80, 62), speck="ruins"),
    "lava": dict(
        floor1=(52, 34, 32), floor2=(38, 25, 25), floor3=(70, 46, 40),
        wall=(34, 22, 22), wallTop=(120, 62, 40), wallDark=(16, 10, 10),
        hazard=(255, 110, 20), accent=(255, 140, 40),
        water=(120, 40, 16), road=(58, 36, 30), speck="lava"),
    "void": dict(
        floor1=(28, 22, 40), floor2=(20, 16, 30), floor3=(40, 32, 58),
        wall=(16, 12, 26), wallTop=(96, 62, 150), wallDark=(8, 6, 14),
        hazard=(180, 110, 255), accent=(200, 140, 255),
        water=(26, 20, 48), road=(38, 30, 54), speck="void"),
    "town": dict(
        floor1=(88, 74, 58), floor2=(72, 60, 48), floor3=(106, 90, 70),
        wall=(56, 46, 38), wallTop=(140, 118, 92), wallDark=(26, 22, 18),
        hazard=(255, 170, 60), accent=(242, 181, 68),
        water=(48, 72, 88), road=(0, 0, 0), speck="town"),   # road: булыжник
}
ORDER = ["ash", "ember", "frost", "crystal", "swamp", "ruins", "lava", "void", "town"]


# ---------------- утилиты цвета ----------------
def shade(c, k):
    return tuple(max(0, min(255, int(round(v * k)))) for v in c[:3])


def mixc(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def rgba(c, a=255):
    return (c[0], c[1], c[2], a)


def rnd_for(tag):
    return random.Random(zlib.crc32(tag.encode("utf-8")) & 0xFFFFFFFF)


# ---------------- рисование на логической сетке ----------------
class Cell:
    """Ячейка 20x20, рисуем прямоугольниками 1x1 (пиксель-арт)."""

    def __init__(self, transparent=False):
        bg = (0, 0, 0, 0) if transparent else (0, 0, 0, 255)
        self.im = Image.new("RGBA", (LOG, LOG), bg)
        self.d = ImageDraw.Draw(self.im)

    def px(self, x, y, c):
        x = int(x); y = int(y)
        if 0 <= x < LOG and 0 <= y < LOG:
            self.d.rectangle([x, y, x, y], fill=c if len(c) == 4 else rgba(c))

    def fill(self, c):
        self.d.rectangle([0, 0, LOG - 1, LOG - 1], fill=rgba(c) if len(c) == 3 else c)

    def rect(self, x0, y0, x1, y1, c):
        self.d.rectangle([x0, y0, x1, y1], fill=rgba(c) if len(c) == 3 else c)

    def line(self, x0, y0, x1, y1, c):
        self.d.line([x0, y0, x1, y1], fill=rgba(c) if len(c) == 3 else c)

    def ellipse(self, cx, cy, rx, ry, c):
        self.d.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], fill=rgba(c) if len(c) == 3 else c)

    def noise(self, base, amp, rnd, alpha=255):
        for y in range(LOG):
            for x in range(LOG):
                self.px(x, y, rgba(tuple(max(0, min(255, base[i] + rnd.randint(-amp, amp)))
                                         for i in range(3)), alpha))

    def blobs(self, base, rnd, n=2, amp=14):
        for _ in range(n):
            bx = rnd.uniform(3, LOG - 4); by = rnd.uniform(3, LOG - 4)
            rx = rnd.uniform(2.5, 5.5); ry = rnd.uniform(1.8, 4.0)
            col = shade(base, 1.0 + (amp / 100.0) * rnd.choice([-1, 1]))
            for y in range(int(by - ry), int(by + ry) + 1):
                for x in range(int(bx - rx), int(bx + rx) + 1):
                    if ((x - bx) / rx) ** 2 + ((y - by) / ry) ** 2 <= 1.0 and rnd.random() < 0.55:
                        self.px(x, y, col)


# ============================================================
#  ПОЛ
# ============================================================
SPECK = {
    "ash":     [((190, 180, 172), 58), ((28, 24, 24), 150)],
    "ember":   [((96, 120, 62), 150), ((24, 30, 18), 150)],
    "frost":   [((240, 248, 255), 120), ((160, 190, 220), 140)],
    "crystal": [((176, 132, 255), 110), ((24, 18, 40), 150)],
    "swamp":   [((70, 96, 44), 130), ((26, 32, 20), 150)],
    "ruins":   [((196, 176, 132), 100), ((50, 44, 36), 150)],
    "lava":    [((16, 10, 12), 170), ((255, 130, 40), 90)],
    "void":    [((220, 190, 255), 150), ((10, 8, 18), 160)],
    "town":    [((150, 120, 80), 90), ((46, 38, 30), 150)],
}


def cell_floor(b, v):
    pal = BIOMES[b]
    rnd = rnd_for("%s-floor%d" % (b, v))
    c = Cell()
    # тон вариантов почти одинаковый (иначе пол распадается на «шахматку»),
    # варианты различаются рисунком, а не светлотой
    base = mixc(pal["floor2"], pal["floor3"], 0.32 + 0.03 * v)
    base = shade(base, 0.995 + 0.004 * v)
    c.noise(base, 5, rnd)
    c.blobs(base, rnd, n=2 + v % 2, amp=11)
    if v >= 1:
        c.blobs(base, rnd, n=1, amp=7)
    # характерные крупинки биома
    cols = SPECK[pal["speck"]]
    for col, alpha in cols:
        for _ in range(rnd.randint(2, 5)):
            x = rnd.randrange(LOG); y = rnd.randrange(LOG)
            c.px(x, y, rgba(shade(col, rnd.uniform(0.8, 1.1)), alpha))
    # у варианта 3 - маленький камешек, у 4 - трещинка
    if v == 3:
        x = rnd.randrange(5, 14); y = rnd.randrange(6, 14)
        c.ellipse(x, y, 2, 1, shade(pal["floor3"], 0.8))
        c.px(x - 1, y - 1, shade(pal["floor3"], 1.25))
    if v == 4:
        x = rnd.randrange(3, 12); y = rnd.randrange(3, 12)
        for i in range(6):
            c.px(x + i, y + (i // 2) * rnd.choice([0, 1]), rgba(pal["wallDark"], 140))
    return c


# ============================================================
#  ДОРОГА
# ============================================================
def cell_road_dirt(pal, v, rnd):
    c = Cell()
    base = shade(mixc(pal["floor2"], pal["road"], 0.65), 0.86 + 0.03 * v)
    c.noise(base, 5, rnd)
    c.blobs(base, rnd, n=2, amp=10)
    # колеи/тропинки
    dark = shade(base, 0.72)
    for yy in (5 + v, 13 - v):
        for x in range(LOG):
            wob = int(round(math.sin(x * 0.7 + v * 2.1) * 1.2))
            c.px(x, yy + wob, dark)
            if rnd.random() < 0.4:
                c.px(x, yy + wob + 1, shade(dark, 1.15))
    # камешки
    for _ in range(4):
        x = rnd.randrange(2, LOG - 2); y = rnd.randrange(2, LOG - 2)
        c.px(x, y, shade(base, 1.4)); c.px(x + 1, y, shade(base, 0.6))
    return c


def cell_road_cobble(v, rnd):
    c = Cell()
    mortar = (44, 40, 36)
    c.fill(tuple(max(0, m + rnd.randint(-4, 4)) for m in mortar))
    stone1, stone2 = (116, 110, 100), (92, 86, 78)
    rows = [(0, 4), (5, 9), (10, 14), (15, 19)]
    for ri, (y0, y1) in enumerate(rows):
        off = (ri + v) % 2 * 4
        x = -off
        while x < LOG:
            w = rnd.randint(5, 8)
            x0 = max(0, x + 1); x1 = min(LOG - 1, x + w - 1)
            if x1 > x0:
                col = shade(stone1 if (ri + x) % 2 == 0 else stone2, rnd.uniform(0.92, 1.08))
                c.rect(x0, y0 + 1, x1, y1 - 1, col)
                c.rect(x0, y0 + 1, x1, y0 + 1, shade(col, 1.18))     # верхний блик
                c.rect(x0, y1 - 1, x1, y1 - 1, shade(col, 0.72))     # нижняя тень
                if v == 1 and ri == 1 and x0 == 5:                    # треснувший камень
                    c.line(x0 + 1, y0 + 2, x1 - 1, y1 - 2, shade(mortar, 0.7))
                if v == 2 and rnd.random() < 0.18:                    # мох в швах
                    c.px(x0, y1 - 1, (74, 92, 52)); c.px(x0 + 1, y1 - 1, (60, 78, 44))
            x += w + 1
    return c


def cell_road(b, v):
    pal = BIOMES[b]
    rnd = rnd_for("%s-road%d" % (b, v))
    if b == "town":
        return cell_road_cobble(v, rnd)
    return cell_road_dirt(pal, v, rnd)


# ============================================================
#  СТЕНЫ
# ============================================================
WALL_ACCENT = {
    "ash":     "soot",     # копоть снизу
    "ember":   "moss",     # мох сверху
    "frost":   "icicle",   # сосульки
    "crystal": "vein",     # аметистовая жила
    "swamp":   "moss",     # мох + потёки
    "ruins":   "sand",     # песочные потёки
    "lava":    "glow",     # светящиеся швы
    "void":    "rune",     # руны
    "town":    "beam",     # деревянная балка
}


def cell_wall(b, v):
    pal = BIOMES[b]
    rnd = rnd_for("%s-wall%d" % (b, v))
    c = Cell()
    c.fill(pal["wallDark"])
    brickH = 5
    for row in range(4):
        y0 = row * brickH
        off = ((row + v) % 2) * 5
        x = -off
        while x < LOG:
            w = 9
            x0 = max(0, x + 1); x1 = min(LOG - 1, x + w - 1)
            if x1 > x0 and y0 < LOG:
                # светлота кирпича варьируется скаляром - оттенок не плывёт
                col = shade(pal["wall"], rnd.uniform(0.86, 1.16))
                y1 = min(LOG - 1, y0 + brickH - 2)
                c.rect(x0, y0, x1, y1, col)
                c.rect(x0, y0, x1, y0, shade(col, 1.4))          # блик сверху
                c.rect(x0, y1, x1, y1, shade(col, 0.65))         # тень снизу
                if v == 1 and row == rnd.randrange(0, 4) and rnd.random() < 0.5:
                    # трещина по кирпичу
                    cx = (x0 + x1) // 2
                    c.line(cx, y0 + 1, cx + rnd.choice([-1, 1]), y1 - 1, shade(pal["wallDark"], 1.3))
            x += w + 1
    # акцент варианта 2 - по биому
    if v == 2:
        acc = WALL_ACCENT[b]
        if acc == "moss":
            for _ in range(10):
                x = rnd.randrange(LOG); y = rnd.randrange(0, 4 if b == "swamp" else 3)
                c.px(x, y, (58, 74, 40) if b == "ember" else (64, 84, 44))
        elif acc == "icicle":
            for x in (3, 9, 15):
                ln = rnd.randint(2, 4)
                for i in range(ln):
                    c.px(x, 4 + i, shade((200, 226, 244), 1.0 - i * 0.12))
        elif acc == "vein":
            x = rnd.randrange(2, 16)
            for y in range(2, LOG - 1):
                c.px(x + (y // 4) % 2, y, rgba(pal["accent"], 170))
        elif acc == "sand":
            for _ in range(12):
                x = rnd.randrange(LOG); y = rnd.randrange(LOG - 6, LOG)
                c.px(x, y, rgba((168, 148, 104), 140))
        elif acc == "glow":
            for row in range(4):
                y = row * brickH + brickH - 1
                for x in range(rnd.randrange(0, 6), LOG, rnd.randint(7, 12)):
                    c.px(x, y, rgba(pal["hazard"], 200))
                    c.px(x + 1, y, rgba(shade(pal["hazard"], 1.3), 130))
        elif acc == "rune":
            rx, ry = rnd.randrange(4, 14), rnd.randrange(6, 14)
            for a in range(0, 360, 40):
                c.px(rx + int(2.4 * math.cos(math.radians(a))), ry + int(2.4 * math.sin(math.radians(a))),
                     rgba(pal["accent"], 160))
        elif acc == "beam":
            bx = 8 + (v % 2)
            c.rect(bx, 0, bx + 3, LOG - 1, (90, 66, 42))
            c.rect(bx, 0, bx, LOG - 1, (58, 42, 26))
            c.rect(bx + 3, 0, bx + 3, LOG - 1, (120, 92, 58))
            for y in (4, 14):
                c.px(bx + 1, y, (40, 30, 20))
        elif acc == "soot":
            for _ in range(14):
                x = rnd.randrange(LOG); y = rnd.randrange(LOG - 7, LOG)
                c.px(x, y, rgba((14, 12, 12), 130))
    return c


def cell_walltop(b, v):
    pal = BIOMES[b]
    rnd = rnd_for("%s-walltop%d" % (b, v))
    c = Cell()
    cap = shade(pal["wallTop"], 0.80)
    face = shade(pal["wallTop"], 0.62)
    c.fill(face)
    for y in range(LOG):
        for x in range(LOG):
            c.px(x, y, shade(face if y > 9 else cap, 1.0 + rnd.uniform(-0.035, 0.035)))
    c.rect(0, 0, LOG - 1, 0, pal["wallDark"])          # кромка сверху
    c.rect(0, 10, LOG - 1, 10, shade(pal["wallDark"], 1.25))  # разделитель
    c.rect(0, LOG - 1, LOG - 1, LOG - 1, pal["wallDark"])     # кромка снизу
    for _ in range(8):                                   # крапинки на шляпке
        x = rnd.randrange(1, LOG - 1); y = rnd.randrange(2, 9)
        c.px(x, y, shade(cap, 1.12))
    for yy in (3, 6):                                    # горизонтальные борозды шляпки
        for x in range(1, LOG - 1):
            if rnd.random() < 0.6:
                c.px(x, yy + (1 if x % 5 == 0 else 0), shade(cap, 0.9))
    for xx in (4, 11, 17):                               # швы на лицевой части
        for y in range(11, LOG - 1):
            c.px(xx, y, shade(face, 0.85))
    for _ in range(6):                                   # крапинки на лице
        x = rnd.randrange(1, LOG - 1); y = rnd.randrange(11, LOG - 1)
        c.px(x, y, shade(face, 1.1))
    if v == 1:                                           # щербатый край + трещина
        for i in range(3):
            c.rect(i, 1, i, 2 + i, face)
        c.px(3, 1, shade(pal["wallDark"], 1.4))
        cx = rnd.randrange(6, 14)
        for y in range(3, 9):
            c.px(cx + (y % 3) - 1, y, shade(pal["wallDark"], 1.5))
    return c


# ============================================================
#  ОПАСНОСТЬ / ВОДА
# ============================================================
def cell_hazard(b, v):
    pal = BIOMES[b]
    rnd = rnd_for("%s-hazard%d" % (b, v))
    c = Cell()
    base = shade(pal["floor2"], 0.38)
    c.noise(base, 3, rnd)
    cx = LOG / 2 + (-3 if v else 3); cy = LOG / 2 + (2 if v else -2)
    hz = pal["hazard"]
    for y in range(LOG):
        for x in range(LOG):
            d = math.hypot(x - cx, y - cy) / (LOG * 0.62)
            t = max(0.0, 1.0 - d) ** 1.5
            if t > 0.02:
                c.px(x, y, mixc(base, shade(hz, 1.25 if t > 0.7 else 1.0), min(1.0, t * 1.15)))
    # мотив биома поверх свечения
    if b in ("ash", "lava", "ember"):       # раскалённые трещины
        for k in range(3):
            x, y = cx + rnd.uniform(-2, 2), cy + rnd.uniform(-2, 2)
            ang = rnd.uniform(0, 2 * math.pi)
            for i in range(7):
                a = max(60, 230 - i * 26)
                c.px(x + i * math.cos(ang), y + i * math.sin(ang),
                     rgba((255, 226, 140) if b == "lava" else shade(hz, 1.35), a))
    elif b == "frost":                      # ледяные шипы
        for sx in (5, 10, 15):
            hh = rnd.randint(3, 6)
            for i in range(hh):
                w = max(0, (hh - i) // 2)
                c.rect(sx - w, 16 - i, sx + w, 16 - i, shade((190, 228, 250), 1.0 - i * 0.05))
    elif b == "crystal":                    # кристальные иглы
        for sx, sy in ((6, 12), (12, 8), (10, 15)):
            for i in range(4):
                c.px(sx, sy - i, shade(pal["accent"], 1.2 - i * 0.1))
                c.px(sx - 1, sy - i + 1, shade(pal["accent"], 0.8))
    elif b == "swamp":                      # булькающая жижа
        for bx, by, r in ((6, 7, 2), (13, 12, 3), (10, 5, 1)):
            for a in range(0, 360, 30):
                c.px(bx + r * math.cos(math.radians(a)), by + r * math.sin(math.radians(a)),
                     shade(hz, 1.25))
    elif b == "ruins":                      # мелкая затопленная плита
        for _ in range(5):
            x = rnd.randrange(3, 16); y = rnd.randrange(3, 16)
            c.rect(x, y, x + 1, y, (86, 96, 88))
    elif b == "void":                       # кольцо-воронка
        for a in range(0, 360, 15):
            for r in (3.5, 6.0):
                c.px(cx + r * math.cos(math.radians(a)), cy + r * math.sin(math.radians(a)),
                     shade(pal["accent"], 1.2 if r < 5 else 0.9))
    else:                                   # town: рассыпанные угли
        for _ in range(8):
            x = rnd.randrange(3, 17); y = rnd.randrange(3, 17)
            c.px(x, y, (255, 190, 90)); c.px(x + 1, y, rgba(hz, 150))
    return c


def cell_water(b, v):
    pal = BIOMES[b]
    rnd = rnd_for("%s-water%d" % (b, v))
    c = Cell()
    wt = pal["water"]
    c.noise(shade(wt, 0.9), 4, rnd)
    light = shade(wt, 1.45)
    for k in range(3):
        yy = 3 + k * 6 + (2 if v else 0)
        for x in range(LOG):
            y = yy + int(round(math.sin(x * 0.55 + k * 2.0 + v) * 1.3))
            c.px(x, y, light)
            if (x + k * 5) % 7 == 0:
                c.px(x, y - 1, shade(light, 1.2))
    if b == "lava":                          # магма: ярче и «тягучее»
        for _ in range(6):
            x = rnd.randrange(2, 18); y = rnd.randrange(2, 18)
            c.px(x, y, (255, 170, 60)); c.px(x + 1, y, (255, 120, 30))
    return c


# ============================================================
#  ДЕТАЛИ ПОЛА (прозрачные)
# ============================================================
def _stone(c, rnd, x, y, r, col):
    c.ellipse(x, y, r, max(1, r - 1), col)
    c.px(x - r + 1, y - 1, shade(col, 1.3))


def cell_deco(b, v):
    pal = BIOMES[b]
    rnd = rnd_for("%s-deco%d" % (b, v))
    c = Cell(transparent=True)
    key = (b, v)

    # ---------- ash ----------
    if key == ("ash", 0):        # трещина
        x, y = 4, 12
        for i in range(11):
            c.px(x + i, y - (i // 3), rgba((20, 16, 16), 190))
            c.px(x + i, y - (i // 3) - 1, rgba((120, 110, 104), 60))
    elif key == ("ash", 1):      # пятно пепла
        for _ in range(26):
            a = rnd.uniform(0, 2 * math.pi); r = rnd.uniform(0, 5.5)
            c.px(10 + r * math.cos(a), 10 + r * math.sin(a) * 0.7, rgba((190, 180, 172), 46))
    elif key == ("ash", 2):      # камешки
        for i, (sx, sy) in enumerate([(6, 8), (12, 11), (9, 14)]):
            _stone(c, rnd, sx, sy, 2 if i != 1 else 1, (48, 42, 42))
    elif key == ("ash", 3):      # осколок кости
        c.line(6, 13, 13, 7, (214, 202, 176))
        c.line(6, 12, 12, 6, rgba((232, 220, 192), 200))
        c.px(13, 6, (168, 156, 134))
    elif key == ("ash", 4):      # тлеющие искры
        for sx, sy in [(6, 9), (12, 6), (10, 13), (15, 11)]:
            c.px(sx, sy, rgba((255, 150, 60), 210)); c.px(sx + 1, sy, rgba((255, 110, 40), 120))
            c.px(sx, sy + 1, rgba((120, 50, 20), 90))
    elif key == ("ash", 5):      # подпалина
        for _ in range(30):
            a = rnd.uniform(0, 2 * math.pi); r = rnd.uniform(0, 6)
            c.px(10 + r * math.cos(a), 10 + r * math.sin(a) * 0.65, rgba((14, 12, 12), int(120 - r * 14)))

    # ---------- ember ----------
    elif key == ("ember", 0):    # пучок травы
        for i, dx in enumerate([-2, 0, 2]):
            hgt = 5 + i
            for j in range(hgt):
                c.px(10 + dx + (j // 3) * (1 if dx >= 0 else -1), 15 - j,
                     rgba(shade((96, 120, 62), 1.0 + j * 0.05), 210))
    elif key == ("ember", 1):    # корень
        for i in range(12):
            c.px(4 + i, 8 + int(round(2.4 * math.sin(i * 0.55))), rgba((58, 44, 30), 200))
            c.px(5 + i, 9 + int(round(2.4 * math.sin(i * 0.55))), rgba((40, 30, 20), 150))
    elif key == ("ember", 2):    # пара грибов
        for sx, sy, r in [(7, 12, 2), (13, 10, 3)]:
            c.rect(sx, sy, sx, sy + 2, (216, 204, 176))
            c.ellipse(sx, sy, r, max(1, r - 1), (150, 90, 60))
            c.px(sx - 1, sy - 1, (196, 130, 84))
    elif key == ("ember", 3):    # опавшие листья
        for _ in range(5):
            x = rnd.randrange(4, 16); y = rnd.randrange(5, 15)
            c.px(x, y, rgba((188, 116, 48), 200)); c.px(x + 1, y, rgba((140, 84, 36), 170))
    elif key == ("ember", 4):    # камешек
        _stone(c, rnd, 10, 11, 3, (74, 70, 62))
        c.px(12, 13, (52, 48, 44))
    elif key == ("ember", 5):    # росток папоротника
        for i in range(7):
            c.px(9 + i // 3, 15 - i, rgba((70, 104, 48), 210))
        c.px(11, 9, rgba((96, 130, 62), 210)); c.px(8, 11, rgba((96, 130, 62), 210))

    # ---------- frost ----------
    elif key == ("frost", 0):    # сугробик
        for _ in range(30):
            a = rnd.uniform(0, 2 * math.pi); r = rnd.uniform(0, 6)
            yy = 12 + r * math.sin(a) * 0.5
            if yy > 8:
                c.px(10 + r * math.cos(a), yy, rgba((240, 248, 255), int(90 - abs(r) * 8)))
    elif key == ("frost", 1):    # ледяная трещина
        x, y = 3, 9
        for i in range(13):
            c.px(x + i, y + (i // 4) * (1 if i % 2 else -1), rgba((200, 235, 255), 150))
    elif key == ("frost", 2):    # кристаллики инея
        for sx, sy in [(7, 8), (13, 12)]:
            for a in range(0, 360, 60):
                for r in range(1, 3):
                    c.px(sx + r * math.cos(math.radians(a)), sy + r * math.sin(math.radians(a)),
                         rgba((250, 252, 255), 220))
    elif key == ("frost", 3):    # мёрзлый камешек
        _stone(c, rnd, 10, 12, 3, (122, 140, 164))
        c.rect(8, 9, 12, 9, rgba((232, 244, 252), 200))
    elif key == ("frost", 4):    # блики-звёздочки
        for sx, sy in [(6, 7), (14, 12), (10, 15)]:
            c.px(sx, sy, rgba((255, 255, 255), 230))
            c.px(sx - 1, sy, rgba((210, 235, 255), 130)); c.px(sx + 1, sy, rgba((210, 235, 255), 130))
            c.px(sx, sy - 1, rgba((210, 235, 255), 130)); c.px(sx, sy + 1, rgba((210, 235, 255), 130))
    elif key == ("frost", 5):    # ледяные иглы
        for sx, hh in [(7, 5), (12, 7), (15, 4)]:
            for i in range(hh):
                c.px(sx, 16 - i, rgba(shade((178, 214, 238), 1.05 - i * 0.03), 220))
                c.px(sx + 1, 16 - i, rgba((120, 160, 196), 150))

    # ---------- crystal ----------
    elif key == ("crystal", 0):  # жила
        for x in range(2, 18):
            y = int(10 + 3 * math.sin(x * 0.5))
            c.px(x, y, rgba((176, 132, 255), 150))
            c.px(x, y + 1, rgba((120, 84, 200), 100))
    elif key == ("crystal", 1):  # друза
        for sx, sy, hh in [(6, 14, 6), (10, 15, 8), (14, 14, 5)]:
            for i in range(hh):
                w = max(0, (hh - i) // 3)
                c.rect(sx - w, sy - i, sx + w, sy - i, rgba(shade((150, 110, 230), 1.0 + i * 0.05), 230))
            c.px(sx, sy - hh + 1, rgba((226, 200, 255), 240))
    elif key == ("crystal", 2):  # руна-кольцо
        for a in range(0, 360, 18):
            c.px(10 + 4.5 * math.cos(math.radians(a)), 10 + 4.5 * math.sin(math.radians(a)),
                 rgba((176, 132, 255), 110))
        c.px(10, 10, rgba((226, 200, 255), 150))
    elif key == ("crystal", 3):  # одиночный кристалл
        for i in range(8):
            w = min(i, 7 - i) // 2
            c.rect(10 - w, 5 + i, 10 + w, 5 + i, rgba(shade((160, 120, 240), 1.0 + (3 - abs(3 - i)) * 0.06), 235))
        c.px(10, 5, rgba((236, 220, 255), 250))
    elif key == ("crystal", 4):  # мерцание
        for sx, sy in [(7, 8), (13, 13)]:
            c.px(sx, sy, rgba((240, 226, 255), 235))
            c.px(sx - 1, sy, rgba((176, 132, 255), 120)); c.px(sx + 1, sy, rgba((176, 132, 255), 120))
            c.px(sx, sy - 1, rgba((176, 132, 255), 120)); c.px(sx, sy + 1, rgba((176, 132, 255), 120))
    elif key == ("crystal", 5):  # жеода
        c.ellipse(10, 11, 4, 3, (30, 24, 46))
        c.ellipse(10, 11, 2, 1, rgba((190, 150, 255), 200))
        c.px(9, 9, rgba((236, 220, 255), 230))

    # ---------- swamp ----------
    elif key == ("swamp", 0):    # мох
        for _ in range(34):
            a = rnd.uniform(0, 2 * math.pi); r = rnd.uniform(0, 6.5)
            c.px(10 + r * math.cos(a), 10 + r * math.sin(a) * 0.75, rgba((70, 96, 44), int(110 - r * 9)))
    elif key == ("swamp", 1):    # пузыри
        for bx, by, r in [(7, 9, 2), (13, 12, 3)]:
            for a in range(0, 360, 30):
                c.px(bx + r * math.cos(math.radians(a)), by + r * math.sin(math.radians(a)),
                     rgba((160, 210, 120), 130))
            c.px(bx - 1, by - 1, rgba((210, 240, 170), 160))
    elif key == ("swamp", 2):    # тина
        for x in range(3, 17):
            y = int(11 + 1.8 * math.sin(x * 0.8))
            c.px(x, y, rgba((46, 74, 34), 170)); c.px(x, y + 1, rgba((34, 56, 26), 120))
    elif key == ("swamp", 3):    # корень
        for i in range(13):
            c.px(3 + i, 10 + int(round(2.6 * math.sin(i * 0.5))), rgba((44, 34, 22), 200))
    elif key == ("swamp", 4):    # камыш
        for sx in (8, 11, 13):
            for i in range(9):
                c.px(sx + (i // 4), 16 - i, rgba((74, 96, 48), 220))
            c.px(sx + 2, 7, rgba((110, 92, 56), 220))
    elif key == ("swamp", 5):    # грязь
        for _ in range(26):
            a = rnd.uniform(0, 2 * math.pi); r = rnd.uniform(0, 5.5)
            c.px(10 + r * math.cos(a), 11 + r * math.sin(a) * 0.6, rgba((32, 26, 18), int(150 - r * 16)))

    # ---------- ruins ----------
    elif key == ("ruins", 0):    # плита
        c.rect(3, 3, 16, 16, rgba((50, 44, 36), 90))
        c.rect(3, 3, 16, 3, rgba((140, 128, 106), 100))
        c.px(3, 16, rgba((50, 44, 36), 130)); c.px(16, 16, rgba((50, 44, 36), 130))
        c.rect(14, 14, 16, 16, rgba((96, 88, 74), 0))   # скол угла
    elif key == ("ruins", 1):    # песок
        for _ in range(28):
            a = rnd.uniform(0, 2 * math.pi); r = rnd.uniform(0, 6)
            c.px(10 + r * math.cos(a), 10 + r * math.sin(a) * 0.7, rgba((196, 176, 132), int(80 - r * 7)))
    elif key == ("ruins", 2):    # треснувшая плитка
        c.line(4, 14, 9, 8, rgba((44, 38, 30), 160)); c.line(9, 8, 15, 10, rgba((44, 38, 30), 160))
        c.px(9, 7, rgba((150, 138, 116), 130))
    elif key == ("ruins", 3):    # щебень
        for sx, sy, r in [(6, 9, 2), (11, 12, 2), (14, 8, 1), (8, 14, 1)]:
            _stone(c, rnd, sx, sy, r, (104, 96, 82))
    elif key == ("ruins", 4):    # трава меж плит
        for dx in (-1, 1, 3):
            for i in range(5):
                c.px(10 + dx + (i // 3), 15 - i, rgba((90, 110, 60), 200))
    elif key == ("ruins", 5):    # резной жёлоб
        c.rect(2, 9, 17, 9, rgba((40, 35, 28), 150))
        c.rect(2, 10, 17, 10, rgba((150, 138, 116), 110))

    # ---------- lava ----------
    elif key == ("lava", 0):     # светящаяся трещина
        x, y = 3, 12
        for i in range(14):
            yy = y - (i // 3)
            c.px(x + i, yy, rgba((255, 200, 90), 230))
            c.px(x + i, yy + 1, rgba((255, 110, 30), 170))
            c.px(x + i, yy - 1, rgba((120, 40, 10), 120))
    elif key == ("lava", 1):     # обсидиан
        c.ellipse(10, 11, 5, 3, (16, 10, 14))
        c.line(6, 9, 10, 8, rgba((90, 60, 80), 160))
        c.px(13, 10, rgba((255, 120, 40), 140))
    elif key == ("lava", 2):     # угли
        for _ in range(7):
            x = rnd.randrange(4, 16); y = rnd.randrange(5, 15)
            c.px(x, y, rgba((196, 60, 24), 200)); c.px(x + 1, y, rgba((90, 26, 12), 160))
    elif key == ("lava", 3):     # лужица магмы
        c.ellipse(10, 11, 4, 2, rgba((255, 120, 30), 220))
        c.ellipse(10, 11, 2, 1, rgba((255, 214, 120), 235))
    elif key == ("lava", 4):     # камень с подсветкой
        _stone(c, rnd, 10, 12, 3, (40, 26, 24))
        c.px(8, 9, rgba((255, 140, 40), 160)); c.px(9, 9, rgba((255, 110, 30), 120))
    elif key == ("lava", 5):     # дымок
        for i in range(8):
            x = 10 + int(round(2.2 * math.sin(i * 0.9)))
            c.px(x, 16 - i * 1.4, rgba((120, 100, 96), int(90 - i * 9)))

    # ---------- void ----------
    elif key == ("void", 0):     # звёзды
        for _ in range(6):
            x = rnd.randrange(3, 17); y = rnd.randrange(3, 17)
            c.px(x, y, rgba((220, 190, 255), rnd.randint(90, 230)))
    elif key == ("void", 1):     # руна-круг
        for a in range(0, 360, 20):
            c.px(10 + 5 * math.cos(math.radians(a)), 10 + 5 * math.sin(math.radians(a)),
                 rgba((180, 120, 255), 110))
        c.px(10, 10, rgba((230, 200, 255), 170))
        c.px(13, 8, rgba((180, 120, 255), 130)); c.px(7, 12, rgba((180, 120, 255), 130))
    elif key == ("void", 2):     # туманность
        for _ in range(30):
            a = rnd.uniform(0, 2 * math.pi); r = rnd.uniform(0, 6)
            c.px(10 + r * math.cos(a), 10 + r * math.sin(a) * 0.7,
                 rgba(mixc((70, 40, 120), (140, 90, 200), r / 6.0), int(70 - r * 7)))
    elif key == ("void", 3):     # парящий осколок
        c.line(8, 12, 13, 7, rgba((24, 18, 38), 240))
        c.line(8, 12, 12, 13, rgba((24, 18, 38), 240))
        c.px(13, 7, rgba((200, 140, 255), 200)); c.px(12, 8, rgba((140, 90, 200), 160))
    elif key == ("void", 4):     # тёмный разлом
        for i in range(10):
            c.px(5 + i, 6 + (i // 2), rgba((6, 4, 10), 220))
            c.px(5 + i, 5 + (i // 2), rgba((90, 60, 140), 90))
    elif key == ("void", 5):     # искра
        c.px(10, 10, rgba((240, 220, 255), 250))
        for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1), (-2, 0), (2, 0), (0, -2), (0, 2)]:
            c.px(10 + dx, 10 + dy, rgba((180, 120, 255), 150 if abs(dx) + abs(dy) == 1 else 80))

    # ---------- town ----------
    elif key == ("town", 0):     # шов досок + гвозди
        c.rect(0, 9, 19, 9, rgba((46, 38, 30), 150))
        c.rect(0, 10, 19, 10, rgba((140, 118, 92), 90))
        c.px(4, 9, rgba((30, 24, 18), 220)); c.px(15, 9, rgba((30, 24, 18), 220))
    elif key == ("town", 1):     # каменный акцент
        c.rect(4, 5, 15, 14, rgba((40, 34, 28), 160))
        c.rect(5, 6, 14, 13, (112, 104, 92))
        c.rect(5, 6, 14, 6, (138, 128, 112)); c.rect(5, 13, 14, 13, (84, 78, 68))
    elif key == ("town", 2):     # просыпанное зерно
        for _ in range(12):
            x = rnd.randrange(5, 15); y = rnd.randrange(7, 14)
            c.px(x, y, rgba((214, 186, 122), 220)); c.px(x, y + 1, rgba((160, 132, 78), 150))
    elif key == ("town", 3):     # уголок ковра
        for i in range(9):
            c.rect(3, 5 + i, 3 + i * 1.4, 5 + i, rgba((146, 62, 52), 230))
        c.rect(3, 5, 5, 13, rgba((110, 44, 38), 230))
        for i in range(4):
            c.px(4 + i * 3, 14, rgba((214, 186, 122), 200))
    elif key == ("town", 4):     # камешки у крыльца
        for sx, sy, r in [(6, 12, 2), (12, 10, 2), (10, 15, 1)]:
            _stone(c, rnd, sx, sy, r, (122, 112, 98))
        c.px(15, 6, rgba((30, 24, 18), 230))
    else:                        # town 5: пыль/песок
        for _ in range(24):
            a = rnd.uniform(0, 2 * math.pi); r = rnd.uniform(0, 5.5)
            c.px(10 + r * math.cos(a), 11 + r * math.sin(a) * 0.6, rgba((168, 142, 104), int(70 - r * 6)))
    return c


# ============================================================
#  СБОРКА АТЛАСА
# ============================================================
def build_atlas(b):
    """Логический атлас (COLS*20 x rows*20), ячейки по NAMES."""
    rows = (len(NAMES) + COLS - 1) // COLS
    atlas = Image.new("RGBA", (COLS * LOG, rows * LOG), (0, 0, 0, 0))
    cells = {}
    for i, name in enumerate(NAMES):
        if name.startswith("floor"):
            c = cell_floor(b, i)
        elif name.startswith("road"):
            c = cell_road(b, i)
        elif name.startswith("walltop"):
            c = cell_walltop(b, i)
        elif name.startswith("wall"):
            c = cell_wall(b, i)
        elif name.startswith("hazard"):
            c = cell_hazard(b, i)
        elif name.startswith("water"):
            c = cell_water(b, i)
        else:
            c = cell_deco(b, i)
        x = (i % COLS) * LOG; y = (i // COLS) * LOG
        atlas.paste(c.im, (x, y), c.im)
        cells[name] = i
    big = atlas.resize((atlas.width * 2, atlas.height * 2), Image.NEAREST)
    return big, cells


def build_mock(b, tilesW=16, tilesH=12):
    """Мок-карта для визуальной проверки: пол, дорога, вода, опасность, стены."""
    pal = BIOMES[b]
    rnd = rnd_for("%s-mock" % b)
    img = Image.new("RGBA", (tilesW * OUT, tilesH * OUT), (10, 8, 12, 255))

    atlas, _ = build_atlas(b)

    def cell_img(name):
        i = NAMES.index(name)
        x = (i % COLS) * OUT; y = (i // COLS) * OUT
        return atlas.crop((x, y, x + OUT, y + OUT))

    def put(name, tx, ty):
        img.paste(cell_img(name), (tx * OUT, ty * OUT), cell_img(name))

    def h(x, y):
        return zlib.crc32(("%d,%d,%s" % (x, y, b)).encode()) & 0x7FFFFFFF

    for ty in range(tilesH):
        for tx in range(tilesW):
            if ty == tilesH - 1:
                put("wall%d" % (h(tx, ty) % 3), tx, ty)
            elif ty == tilesH - 2:
                put("walltop%d" % (h(tx, ty) % 2), tx, ty)
            elif tx in (0, tilesW - 1) and ty > 1:
                put("wall%d" % (h(tx, ty) % 3), tx, ty)
            elif 2 <= tx <= 4 and 7 <= ty <= 8:
                put("water%d" % (h(tx, ty) % 2), tx, ty)
            elif 11 <= tx <= 12 and 8 <= ty <= 9:
                put("hazard%d" % (h(tx, ty) % 2), tx, ty)
            elif ty in (3, 4):
                put("road%d" % (h(tx, ty) % 3), tx, ty)
            else:
                put("floor%d" % (h(tx, ty) % 5), tx, ty)
                if h(tx * 7 + 1, ty * 13 + 5) % 100 < 24:
                    deco = cell_img("deco%d" % (h(tx, ty) % 6))
                    img.paste(deco, (tx * OUT, ty * OUT), deco)
    # «небо» сверху: две верхние строки - масса стены
    for ty in (0, 1):
        for tx in range(tilesW):
            put("wall%d" % (h(tx, ty) % 3), tx, ty)
    return img


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "assets", "tiles")
    prevdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "preview")
    os.makedirs(outdir, exist_ok=True)
    os.makedirs(prevdir, exist_ok=True)

    mocks = []
    for b in ORDER:
        atlas, cells = build_atlas(b)
        png = os.path.join(outdir, "tiles_%s.png" % b)
        atlas.save(png)
        meta = {"key": "tiles_%s" % b, "cell": OUT, "cols": COLS, "cells": cells}
        with open(os.path.join(outdir, "tiles_%s.json" % b), "w", encoding="utf-8") as f:
            json.dump(meta, f, ensure_ascii=False, indent=1, sort_keys=True)
        # превью: атлас x3 на тёмной подложке (видны альфа-ячейки) и мок-карта
        big3 = atlas.resize((atlas.width * 3, atlas.height * 3), Image.NEAREST)
        bg = Image.new("RGBA", big3.size, (26, 22, 36, 255))
        bg.paste(big3, (0, 0), big3)
        bg.save(os.path.join(prevdir, "tiles_%s_atlas.png" % b))
        mock = build_mock(b)
        mock.save(os.path.join(prevdir, "tiles_%s_mock.png" % b))
        mocks.append((b, mock))
        print("tiles_%s: %dx%d, %d ячеек" % (b, atlas.width, atlas.height, len(cells)))

    # общий монтаж 3x3 из мок-карт (уменьшенных)
    mw = mocks[0][1].width // 2; mh = mocks[0][1].height // 2
    allm = Image.new("RGBA", (mw * 3 + 20, mh * 3 + 20), (12, 10, 16, 255))
    for i, (b, m) in enumerate(mocks):
        allm.paste(m.resize((mw, mh), Image.NEAREST), ((i % 3) * (mw + 10), (i // 3) * (mh + 10)))
    allm.save(os.path.join(prevdir, "tiles_all.png"))
    print("готово: %s (+ превью в tools/preview)" % outdir)


if __name__ == "__main__":
    main()
