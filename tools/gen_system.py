#!/usr/bin/env python3
"""Generate docs/img/system-overview.svg — block diagram of the whole display.

Usage: gen_system.py <Igpp.c> <out.svg>
The 3x3 panel field shows frameBuffer[1] from the firmware (96x96, column-major,
12 bytes per column, MSB = top row, bit 0 = lit dot).
"""
import re
import sys

src, dst = sys.argv[1], sys.argv[2]

# ---------------------------------------------------------------- frame from firmware
body = open(src, encoding="utf-8", errors="replace").read()
init = body[body.index("frameBuffer"):]
init = init[:init.index("};")]
frame = [int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{2})", init)][1152:2304]
assert len(frame) == 1152, len(frame)


def lit(x, y):
    b = frame[x * 12 + (y >> 3)]
    return ((b >> (7 - (y & 7))) & 1) == 0  # anodes are active-low


W, H = 1600, 1200
FONT = "'JetBrains Mono','DejaVu Sans Mono',Consolas,monospace"
C = dict(bg="#f6f3ea", grid="#e6e0d0", ink="#1f2430", mute="#8a8778",
         glow="#ff8a1e", dim="#4a2f1f", panel="#1d1410", panel_edge="#0d0806",
         anode="#e8590c", cath="#0e7c86", hv="#c92a2a", v24="#b08900", v33="#d6336c",
         mcu="#14532d", box="#ffffff")

out = []
a = out.append


def text(x, y, s, size=13, fill=None, anchor="start", weight="normal", extra=""):
    a(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill or C["ink"]}" '
      f'text-anchor="{anchor}" font-weight="{weight}" {extra}>{s}</text>')


def box(x, y, w, h, stroke, title, lines=(), fill=None, tsize=14, rx=8):
    a(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill or C["box"]}" '
      f'stroke="{stroke}" stroke-width="2.2"/>')
    a(f'<rect x="{x}" y="{y}" width="{w}" height="6" rx="3" fill="{stroke}"/>')
    text(x + w / 2, y + 28, title, tsize, C["ink"], "middle", "700")
    for i, s in enumerate(lines):
        text(x + w / 2, y + 48 + i * 17, s, 11.5, C["mute"], "middle")


def arrow_line(points, color, width=2.4, dash=None, head=True):
    d = "M" + " L".join(f"{x} {y}" for x, y in points)
    da = f' stroke-dasharray="{dash}"' if dash else ""
    mk = f' marker-end="url(#arr-{color[1:]})"' if head else ""
    a(f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{width}" '
      f'stroke-linejoin="round" stroke-linecap="round"{da}{mk}/>')


def pill(x, y, s, color, fill):
    w = len(s) * 7.2 + 16
    a(f'<rect x="{x - w/2}" y="{y - 10}" width="{w}" height="20" rx="10" fill="{fill}" stroke="{color}" stroke-width="1.3"/>')
    text(x, y + 4.5, s, 11.5, color, "middle", "700")


markers = {C[k] for k in ("anode", "cath", "hv", "v24", "v33", "ink", "mute")}

a(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="{FONT}">')
a('<title>ГИП10000 на стероидах — структурная схема</title>')
a('<defs>')
a('<pattern id="grid" width="20" height="20" patternUnits="userSpaceOnUse">'
  f'<path d="M20 0H0V20" fill="none" stroke="{C["grid"]}" stroke-width="0.7"/></pattern>')
for col in markers:
    a(f'<marker id="arr-{col[1:]}" viewBox="0 0 10 10" refX="8.5" refY="5" markerWidth="7" markerHeight="7" '
      f'orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="{col}"/></marker>')
a('<filter id="glow" x="-50%" y="-50%" width="200%" height="200%">'
  '<feGaussianBlur stdDeviation="1.6" result="b"/><feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter>')
TILE, GAP = 176, 14
PITCH = TILE / 32
a(f'<pattern id="dots" width="{PITCH}" height="{PITCH}" patternUnits="userSpaceOnUse">'
  f'<circle cx="{PITCH/2}" cy="{PITCH/2}" r="1.7" fill="{C["dim"]}"/></pattern>')
a('</defs>')
a(f'<rect width="{W}" height="{H}" fill="{C["bg"]}"/>')
a(f'<rect width="{W}" height="{H}" fill="url(#grid)" opacity="0.6"/>')
a(f'<rect x="14" y="14" width="{W-28}" height="{H-28}" fill="none" stroke="{C["ink"]}" stroke-width="1.6"/>')
text(40, 56, "ГИП10000 на стероидах · структурная схема", 24, C["ink"], "start", "700")
text(40, 82, "9 панелей ИГПП-32/32 в поле 3×3 · 96×96 точек · 600×600 мм", 14, C["mute"])

# ---------------------------------------------------------------- panel field
GX, GY = 660, 140


def tile_xy(r, c):
    return GX + c * (TILE + GAP), GY + r * (TILE + GAP)


for r in range(3):
    for c in range(3):
        x, y = tile_xy(r, c)
        a(f'<rect x="{x-5}" y="{y-5}" width="{TILE+10}" height="{TILE+10}" rx="6" fill="{C["panel_edge"]}"/>')
        a(f'<rect x="{x}" y="{y}" width="{TILE}" height="{TILE}" fill="{C["panel"]}"/>')
        a(f'<rect x="{x}" y="{y}" width="{TILE}" height="{TILE}" fill="url(#dots)"/>')
a(f'<g fill="{C["glow"]}" filter="url(#glow)">')
for px in range(96):
    for py_ in range(96):
        if lit(px, py_):
            r, c = py_ // 32, px // 32
            x0, y0 = tile_xy(r, c)
            cx = x0 + (px % 32) * PITCH + PITCH / 2
            cy = y0 + (py_ % 32) * PITCH + PITCH / 2
            a(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="2.1"/>')
a('</g>')
for r in range(3):
    for c in range(3):
        x, y = tile_xy(r, c)
        a(f'<rect x="{x+4}" y="{y+TILE-22}" width="92" height="18" rx="3" fill="#000" opacity="0.55"/>')
        text(x + 10, y + TILE - 9, f"ИГПП {r+1}.{c+1}", 11, "#ffd8a8", "start", "700")
FX1 = GX + 3 * TILE + 2 * GAP
FY1 = GY + 3 * TILE + 2 * GAP
text(FX1, GY - 14, "на панелях — кадр из frameBuffer[1] прошивки MSP430", 11.5, C["mute"], "end")

# ---------------------------------------------------------------- anode groups (left)
AX, AW = 430, 170
BX, BW = 215, 150
RAIL = 160
for r in range(3):
    _, y = tile_xy(r, 0)
    gy, gh = y + 14, TILE - 28
    box(AX, gy, AW, gh, C["anode"], f"Анодная группа {r+1}",
        ["32 анода · строки", f"{32*r}–{32*r+31}", "ключи + удвоители"])
    pill(AX + AW / 2, gy + gh - 16, "3.3V лог.", C["v33"], "#fde2ec")
    # anode bus to panel row
    by = gy + gh / 2
    a(f'<line x1="{AX+AW}" y1="{by}" x2="{GX-6}" y2="{by}" stroke="{C["anode"]}" stroke-width="7" opacity="0.85"/>')
    a(f'<line x1="{AX+AW+26}" y1="{by+9}" x2="{AX+AW+36}" y2="{by-9}" stroke="{C["ink"]}" stroke-width="1.6"/>')
    text(AX + AW + 8, by - 10, "32", 11.5, C["ink"], "start", "700")
    # boost
    box(BX, gy + 10, BW, gh - 20, C["hv"], "DC/DC boost", ["24 → 200 В", "50–70 мА"], fill="#fff5f5")
    arrow_line([(BX + BW, by), (AX - 4, by)], C["hv"], 3.2)
    text((BX + BW + AX) / 2, by - 10, "+200 В", 12, C["hv"], "middle", "700")
    arrow_line([(RAIL, by), (BX - 4, by)], C["v24"], 2.6)

# anode chain (DI enters bottom group, goes up — per firmware byte order)
for r in (2, 1):
    _, y = tile_xy(r, 0)
    _, yp = tile_xy(r - 1, 0)
    arrow_line([(AX + 26, y + 14), (AX + 26, yp + TILE - 14 + 2)], C["anode"], 2, "5 4")
text(AX + 32, tile_xy(1, 0)[1] + 4, "DO→DI", 10.5, C["anode"], "start", "700")
text(AX + 32, tile_xy(2, 0)[1] + 4, "DO→DI", 10.5, C["anode"], "start", "700")

# ---------------------------------------------------------------- cathode groups (bottom)
CY, CH = FY1 + 60, 120
for c in range(3):
    x, _ = tile_xy(0, c)
    gx, gw = x + 8, TILE - 16
    box(gx, CY, gw, CH, C["cath"], f"Катодная группа {c+1}",
        ["32 катода · столбцы", f"{32*c}–{32*c+31}", "ключи на GND"])
    pill(gx + gw / 2, CY + CH - 14, "3.3V лог.", C["v33"], "#fde2ec")
    bx = gx + gw / 2
    a(f'<line x1="{bx}" y1="{CY}" x2="{bx}" y2="{FY1+6}" stroke="{C["cath"]}" stroke-width="7" opacity="0.85"/>')
    a(f'<line x1="{bx-9}" y1="{CY-12}" x2="{bx+9}" y2="{CY-22}" stroke="{C["ink"]}" stroke-width="1.6"/>')
    text(bx + 12, CY - 14, "32", 11.5, C["ink"], "start", "700")
# cathode chain enters right group, goes left (realCathodePos)
for c in (2, 1):
    x, _ = tile_xy(0, c)
    xp, _ = tile_xy(0, c - 1)
    arrow_line([(x + 8, CY + CH / 2 + 6), (xp + TILE - 8 + 2, CY + CH / 2 + 6)], C["cath"], 2, "5 4")

text((GX + FX1) / 2, CY + CH + 20, "← цепочка катодов: DI входит в группу 3", 11.5, C["cath"], "middle", "700")

# ---------------------------------------------------------------- MCU, power
MX, MY, MW, MH = 360, 960, 270, 160
box(MX, MY, MW, MH, C["mcu"], "ESP32-S3-DevKitC-1",
    ["N16R8 · ESP-IDF", "скан: SPI2 + GPIO", "microSD на SPI3", "кнопка BOOT"], fill="#effaf2")
# Up block path
arrow_line([(MX + 70, MY), (MX + 70, 900), (AX + 60, 900), (AX + 60, tile_xy(2, 0)[1] + TILE - 14 + 2)],
           C["anode"], 2.4, "7 4")
text(AX + 50, 888, "Up block: DI · SRCLK · LATCH · MR", 11.5, C["anode"], "end", "700")
# Down block path
x3, _ = tile_xy(0, 2)
arrow_line([(MX + MW, MY + 40), (x3 + TILE / 2, MY + 40), (x3 + TILE / 2, CY + CH + 2)], C["cath"], 2.4, "7 4")
text(MX + MW + 14, MY + 30, "Down block: DI · SRCLK · LATCH · MR", 11.5, C["cath"], "start", "700")

# USB / Wi-Fi
arrow_line([(180, MY + 140), (MX - 4, MY + 140)], C["mute"], 2, None)
box(36, MY + 108, 144, 60, C["mute"], "ПК", ["USB: поток кадров"], tsize=13)
text(MX + MW - 12, MY + MH - 10, "Wi-Fi · OTA", 11, C["mute"], "end")

# 24 V input and buck
box(40, 120, 110, 70, C["v24"], "+24 В", ["вход питания"], fill="#fff9db")
arrow_line([(150, 155), (RAIL, 155), (RAIL, 1010), (175, 1010)], C["v24"], 3.2, head=False)
a(f'<circle cx="{RAIL}" cy="155" r="4.5" fill="{C["v24"]}"/>')
for r in range(3):
    _, y = tile_xy(r, 0)
    a(f'<circle cx="{RAIL}" cy="{y + 14 + (TILE-28)/2}" r="4.5" fill="{C["v24"]}"/>')
text(RAIL - 8, 600, "шина +24 В", 12, C["v24"], "middle", "700", f'transform="rotate(-90 {RAIL-8} 600)"')
box(175, 965, 130, 90, C["v33"], "DC/DC buck", ["24 → 3.3 В", "ESP32 + логика", "всех 6 групп"], fill="#fff0f6")
arrow_line([(305, 1010), (MX - 4, 1010)], C["v33"], 2.6)
text((305 + MX) / 2, 1000, "3.3 В", 11.5, C["v33"], "middle", "700")

# ---------------------------------------------------------------- notes
NX, NY, NW = FX1 + 40, GY, W - 40 - (FX1 + 40)
a(f'<rect x="{NX}" y="{NY}" width="{NW}" height="560" rx="8" fill="#ffffff" stroke="{C["ink"]}" stroke-width="1.4"/>')
notes = [
    ("Развёртка", True),
    ("Катоды — бегущая единица", False),
    ("по 96 столбцам; аноды — 96 бит", False),
    ("(12 байт) на столбец.", False),
    ("Скважность 1:96 (даташит 1:64).", False),
    ("", False),
    ("Тайминги не менять", True),
    ("На анодах импульсные удвоители:", False),
    ("энергия разряда — только из", False),
    ("конденсатора. Больше ёмкость →", False),
    ("больше ток и нагрев катодных ключей.", False),
    ("", False),
    ("Питание", True),
    ("HV: 3 × 200 В × 70 мА ≈ 42 Вт.", False),
    ("По +24 В с учётом КПД ≈ 2 А —", False),
    ("БП 24 В от 60 Вт.", False),
    ("3.3 В: ESP32 (до ~0,5 А с Wi-Fi)", False),
    ("+ логика групп.", False),
    ("", False),
    ("Цепочки регистров", True),
    ("По одной на аноды и на катоды.", False),
    ("Направление выведено из кода", False),
    ("(порядок байтов, realCathodePos) —", False),
    ("сверить на железе.", False),
    ("", False),
    ("OE групп — жёстко на 3.3 В", True),
    ("MR — 10k на GND", True),
]
yy = NY + 30
for s, bold in notes:
    if s:
        text(NX + 18, yy, s, 13 if bold else 11.5, C["ink"], "start", "700" if bold else "normal")
    yy += 20

# legend
LY = NY + 590
a(f'<rect x="{NX}" y="{LY}" width="{NW}" height="190" rx="8" fill="#ffffff" stroke="{C["ink"]}" stroke-width="1.4"/>')
text(NX + 18, LY + 28, "Обозначения", 14, C["ink"], "start", "700")
leg = [(C["v24"], "+24 В", None), (C["v33"], "+3.3 В", None), (C["hv"], "+200 В на аноды", None),
       (C["anode"], "аноды: шина ×32 / управление", "7 4"), (C["cath"], "катоды: шина ×32 / управление", "7 4"),
       (C["glow"], "горящая точка (кадр из прошивки)", "dot")]
for i, (col, s, kind) in enumerate(leg):
    y = LY + 54 + i * 22
    if kind == "dot":
        a(f'<circle cx="{NX+33}" cy="{y}" r="4" fill="{col}" filter="url(#glow)"/>')
    else:
        da = f' stroke-dasharray="{kind}"' if kind else ""
        a(f'<line x1="{NX+18}" y1="{y}" x2="{NX+48}" y2="{y}" stroke="{col}" stroke-width="3"{da}/>')
    text(NX + 60, y + 4.5, s, 12)

# title block
tb_w, tb_h = 330, 46
a(f'<rect x="{W-14-tb_w}" y="{H-14-tb_h}" width="{tb_w}" height="{tb_h}" fill="#ffffff" stroke="{C["ink"]}" stroke-width="1.6"/>')
text(W - 14 - tb_w + 12, H - 14 - 26, "radiolok/igpp32_32 · esp-migration", 12, C["ink"], "start", "700")
text(W - 14 - tb_w + 12, H - 14 - 9, "структурная схема · лист 2 · rev A", 11, C["mute"])
a('</svg>')
open(dst, "w").write("\n".join(out))
