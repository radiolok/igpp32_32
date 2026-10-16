#!/usr/bin/env python3
"""Generate docs/esp32-wiring.svg: ESP32-S3-DevKitC-1 <-> Up/Down block <-> microSD."""
import sys

W, H = 1480, 1100
PITCH = 34
Y0 = 200                      # y of header pin #1
BX0, BX1 = 600, 860           # board body
PJ1, PJ3 = 614, 846           # pad x for J1 / J3

C = dict(
    bg="#f6f3ea", grid="#e6e0d0", ink="#1f2430", mute="#8a8778", board="#14532d",
    board_edge="#0b3b1f", silk="#e9f5ec", pad="#d4a72c",
    cath="#0e7c86", anode="#e8590c", sd="#7048e8", pwr="#c92a2a", gnd="#343a40",
)
FONT = "'JetBrains Mono','DejaVu Sans Mono',Consolas,monospace"

J1 = ["3V3", "3V3", "RST", "IO4", "IO5", "IO6", "IO7", "IO15", "IO16", "IO17",
      "IO18", "IO8", "IO3", "IO46", "IO9", "IO10", "IO11", "IO12", "IO13", "IO14", "5V", "GND"]
J3 = ["GND", "TX", "RX", "IO1", "IO2", "IO42", "IO41", "IO40", "IO39", "IO38",
      "IO37", "IO36", "IO35", "IO0", "IO45", "IO48", "IO47", "IO21", "IO20", "IO19", "GND", "GND"]


def py(n):
    return Y0 + (n - 1) * PITCH


out = []
a = out.append


def text(x, y, s, size=13, fill=None, anchor="start", weight="normal", extra=""):
    a(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill or C["ink"]}" '
      f'text-anchor="{anchor}" font-weight="{weight}" {extra}>{s}</text>')


def wire(x1, y, x2, color, label=None, lx=None):
    a(f'<line x1="{x1}" y1="{y}" x2="{x2}" y2="{y}" stroke="{color}" stroke-width="2.6" stroke-linecap="round"/>')
    if label:
        lx = lx if lx is not None else (x1 + x2) / 2
        w = len(label) * 7.4 + 12
        a(f'<rect x="{lx - w/2}" y="{y - 9}" width="{w}" height="18" rx="9" fill="{C["bg"]}" stroke="{color}" stroke-width="1.2"/>')
        text(lx, y + 4.5, label, 11.5, color, "middle", "600")


def net(x, y, name, direction):
    """Power net label: short stub and a pill. direction +1 = to the right, -1 = left."""
    color = C["pwr"] if name.startswith("+") else C["gnd"]
    stub = 26
    x2 = x + direction * stub
    a(f'<line x1="{x}" y1="{y}" x2="{x2}" y2="{y}" stroke="{color}" stroke-width="2.2"/>')
    w = len(name) * 7.6 + 16
    rx = x2 if direction > 0 else x2 - w
    fill = "#fde2e2" if color == C["pwr"] else "#dee2e6"
    a(f'<rect x="{rx}" y="{y - 10}" width="{w}" height="20" rx="4" fill="{fill}" stroke="{color}" stroke-width="1.4"/>')
    text(rx + w / 2, y + 4.5, name, 12, color, "middle", "700")


def gnd_symbol(x, y):
    a(f'<g stroke="{C["gnd"]}" stroke-width="2.2" stroke-linecap="round">'
      f'<line x1="{x-12}" y1="{y}" x2="{x+12}" y2="{y}"/>'
      f'<line x1="{x-7}" y1="{y+5}" x2="{x+7}" y2="{y+5}"/>'
      f'<line x1="{x-3}" y1="{y+10}" x2="{x+3}" y2="{y+10}"/></g>')


def pulldown(x, y, color, ref):
    """Junction on wire at (x,y), IEC resistor 10k down to GND."""
    a(f'<circle cx="{x}" cy="{y}" r="4.2" fill="{color}"/>')
    top, rt, rb, bot = y, y + 12, y + 46, y + 56
    a(f'<line x1="{x}" y1="{top}" x2="{x}" y2="{rt}" stroke="{C["gnd"]}" stroke-width="2"/>')
    a(f'<rect x="{x-7}" y="{rt}" width="14" height="{rb-rt}" fill="{C["bg"]}" stroke="{C["gnd"]}" stroke-width="2"/>')
    a(f'<line x1="{x}" y1="{rb}" x2="{x}" y2="{bot}" stroke="{C["gnd"]}" stroke-width="2"/>')
    gnd_symbol(x, bot)
    text(x + 13, rt + 15, ref, 12, C["gnd"], "start", "700")
    text(x + 13, rt + 30, "10k", 12, C["gnd"])


def connector(x, title, sub, first_pin, pins, color, side):
    """pins: list of (name, kind) kind in sig/3v3/gnd/oe. side: pads on 'right' or 'left' edge."""
    top = py(first_pin) - 26
    bot = py(first_pin + len(pins) - 1) + 26
    w = 150
    x0 = x - w if side == "right" else x
    a(f'<rect x="{x0}" y="{top}" width="{w}" height="{bot-top}" rx="8" fill="#ffffff" stroke="{color}" stroke-width="2.4"/>')
    a(f'<rect x="{x0}" y="{top}" width="{w}" height="6" rx="3" fill="{color}"/>')
    text(x0 + w / 2, top - 30, title, 16, C["ink"], "middle", "700")
    text(x0 + w / 2, top - 12, sub, 12, C["mute"], "middle")
    for i, (name, kind) in enumerate(pins):
        y = py(first_pin + i)
        if side == "right":
            text(x0 + 14, y + 4.5, f"{i+1}", 11, C["mute"])
            text(x - 22, y + 4.5, name, 13, C["ink"], "end", "600")
        else:
            text(x0 + w - 14, y + 4.5, f"{i+1}", 11, C["mute"], "end")
            text(x + 22, y + 4.5, name, 13, C["ink"], "start", "600")
        a(f'<rect x="{x-6}" y="{y-6}" width="12" height="12" rx="2" fill="{C["pad"]}" stroke="#8a6d1a" stroke-width="1"/>')


# ---------------------------------------------------------------- document
a(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" '
  f'font-family="{FONT}">')
a('<title>ГИП10000 — переходник ESP32-S3-DevKitC-1</title>')
a('<defs><pattern id="grid" width="20" height="20" patternUnits="userSpaceOnUse">'
  f'<path d="M20 0H0V20" fill="none" stroke="{C["grid"]}" stroke-width="0.7"/></pattern></defs>')
a(f'<rect width="{W}" height="{H}" fill="{C["bg"]}"/>')
a(f'<rect width="{W}" height="{H}" fill="url(#grid)" opacity="0.6"/>')
a(f'<rect x="14" y="14" width="{W-28}" height="{H-28}" fill="none" stroke="{C["ink"]}" stroke-width="1.6"/>')

text(40, 56, "ГИП10000 на стероидах · переходник на ESP32-S3", 24, C["ink"], "start", "700")
text(40, 82, "ESP32-S3-DevKitC-1 N16R8 ↔ разъёмы Up block / Down block ↔ модуль microSD (SPI)", 14, C["mute"])

# ---------------------------------------------------------------- board
a(f'<rect x="{BX0}" y="{py(1)-60}" width="{BX1-BX0}" height="{py(22)-py(1)+120}" rx="10" '
  f'fill="{C["board"]}" stroke="{C["board_edge"]}" stroke-width="2"/>')
# module with antenna
mx0, mx1 = 676, 784
a(f'<rect x="{mx0}" y="{py(1)-50}" width="{mx1-mx0}" height="190" rx="4" fill="#c9ccd1" stroke="#868e96" stroke-width="1.5"/>')
a(f'<rect x="{mx0}" y="{py(1)-50}" width="{mx1-mx0}" height="44" rx="4" fill="#212529"/>')
ax = mx0 + 12
d = f"M{ax} {py(1)-14}"
for k in range(6):
    xx = ax + 14 * k
    d += f" V{py(1)-40} H{xx+7} V{py(1)-14} H{xx+14}"
a(f'<path d="{d}" fill="none" stroke="{C["pad"]}" stroke-width="2"/>')
text((mx0 + mx1) / 2, py(1) + 40, "ESP32-S3", 13, "#343a40", "middle", "700")
text((mx0 + mx1) / 2, py(1) + 58, "WROOM-1", 12, "#343a40", "middle")
text((mx0 + mx1) / 2, py(1) + 76, "N16R8", 12, "#343a40", "middle")
# board name along center
text((BX0 + BX1) / 2, py(12), "ESP32-S3-DevKitC-1", 14, C["silk"], "middle", "700",
     f'transform="rotate(-90 {(BX0+BX1)/2} {py(12)})"')
# buttons
for i, lbl in enumerate(["BOOT", "RST"]):
    bx = 690 + i * 56
    a(f'<rect x="{bx}" y="{py(17)}" width="26" height="20" rx="3" fill="#dee2e6" stroke="#495057"/>')
    a(f'<circle cx="{bx+13}" cy="{py(17)+10}" r="5" fill="#495057"/>')
    text(bx + 13, py(17) + 36, lbl, 11, C["silk"], "middle", "700")
# USB ports
for i in range(2):
    ux = 668 + i * 82
    a(f'<rect x="{ux}" y="{py(22)+42}" width="42" height="30" rx="5" fill="#adb5bd" stroke="#495057" stroke-width="1.5"/>')
text((BX0 + BX1) / 2, py(22) + 92, "UART — прошивка, консоль", 12, C["mute"], "middle")
text((BX0 + BX1) / 2, py(22) + 110, "USB — поток кадров", 12, C["mute"], "middle")

used_j1 = {4: "cath", 6: "cath", 7: "cath", 8: "cath", 16: "anode", 18: "anode", 19: "anode", 20: "anode",
           1: "pwr", 22: "gnd"}
used_j3 = {6: "sd", 7: "sd", 8: "sd", 9: "sd", 1: "gnd", 14: "btn"}
psram = {11, 12, 13}
for n, name in enumerate(J1, 1):
    y = py(n)
    used = n in used_j1
    a(f'<circle cx="{PJ1}" cy="{y}" r="6.5" fill="{C["pad"] if used else "#6c8f77"}" stroke="#0b3b1f" stroke-width="1"/>')
    text(PJ1 + 14, y + 4.5, name, 12.5, C["silk"] if used else "#8fb39a", "start", "700" if used else "normal")
    text(BX0 - 8, y - 7, str(n), 9.5, C["mute"], "end")
for n, name in enumerate(J3, 1):
    y = py(n)
    used = n in used_j3
    col = "#5a3e3e" if n in psram else ("#6c8f77" if not used else C["pad"])
    a(f'<circle cx="{PJ3}" cy="{y}" r="6.5" fill="{col}" stroke="#0b3b1f" stroke-width="1"/>')
    lc = C["silk"] if used else ("#c79a9a" if n in psram else "#8fb39a")
    text(PJ3 - 14, y + 4.5, name, 12.5, lc, "end", "700" if used else "normal")
    text(BX1 + 8, y - 7, str(n), 9.5, C["mute"])
text(BX0 + 14, py(1) - 34, "J1", 12, C["silk"], "start", "700")
text(BX1 - 14, py(1) - 34, "J3", 12, C["silk"], "end", "700")
text(PJ3 - 70, py(12) + 4.5, "PSRAM", 10.5, "#c79a9a", "end")

# ---------------------------------------------------------------- Down block (cathodes)
CX = 300
down = [("3.3V", "3v3"), ("DI", "sig"), ("OE", "oe"), ("LATCH", "sig"),
        ("SRCLK", "sig"), ("MR", "sig"), ("GND", "gnd")]
connector(CX, "Down block", "катоды · разъём платы драйверов", 3, down, C["cath"], "right")
sigs_down = {4: ("CATH_DI", "IO4"), 6: ("CATH_LATCH", "IO6"), 7: ("CATH_SRCLK", "IO7"), 8: ("CATH_MR", "IO15")}
for n, (lbl, _) in sigs_down.items():
    wire(CX + 6, py(n), PJ1 - 7, C["cath"], lbl, 420)
net(CX + 6, py(3), "+3.3V", 1)
net(CX + 6, py(5), "+3.3V", 1)
net(CX + 6, py(9), "GND", 1)
pulldown(500, py(8), C["cath"], "R1")

# ---------------------------------------------------------------- Up block (anodes)
up = [("3.3V", "3v3"), ("DI", "sig"), ("OE", "oe"), ("LATCH", "sig"),
      ("SRCLK", "sig"), ("MR", "sig"), ("GND", "gnd")]
connector(CX, "Up block", "аноды · разъём платы драйверов", 15, up, C["anode"], "right")
sigs_up = {16: "ANODE_DI", 18: "ANODE_LATCH", 19: "ANODE_SRCLK", 20: "ANODE_MR"}
for n, lbl in sigs_up.items():
    wire(CX + 6, py(n), PJ1 - 7, C["anode"], lbl, 420)
net(CX + 6, py(15), "+3.3V", 1)
net(CX + 6, py(17), "+3.3V", 1)
net(CX + 6, py(21), "GND", 1)
pulldown(500, py(20), C["anode"], "R2")

# board power pins
net(PJ1 - 7, py(1), "+3.3V", -1)
net(PJ1 - 7, py(22), "GND", -1)
net(PJ3 + 7, py(1), "GND", 1)

# ---------------------------------------------------------------- microSD module
SX = 1150
sdpins = [("3V3", "3v3"), ("CS", "sig"), ("MISO", "sig"), ("MOSI", "sig"), ("SCK", "sig"), ("GND", "gnd")]
connector(SX, "Модуль microSD", "без LDO и буфера · только 3.3V", 5, sdpins, C["sd"], "left")
sigs_sd = {6: "SD_CS", 7: "SD_MISO", 8: "SD_MOSI", 9: "SD_SCK"}
for n, lbl in sigs_sd.items():
    wire(PJ3 + 7, py(n), SX - 6, C["sd"], lbl, 1010)
net(SX - 6, py(5), "+3.3V", -1)
net(SX - 6, py(10), "GND", -1)
# microSD slot drawing inside module box
a(f'<rect x="{SX+56}" y="{py(5)-6}" width="64" height="{py(10)-py(5)+12}" rx="4" fill="#e9ecef" stroke="#868e96"/>')
for k in range(8):
    a(f'<rect x="{SX+62+k*7}" y="{py(5)+2}" width="4" height="22" fill="{C["pad"]}"/>')
text(SX + 88, py(9) + 10, "microSD", 11, C["mute"], "middle")

# GPIO0 note
a(f'<line x1="{PJ3+7}" y1="{py(14)}" x2="{PJ3+40}" y2="{py(14)}" stroke="{C["mute"]}" stroke-width="1.4" stroke-dasharray="4 3"/>')
text(PJ3 + 46, py(14) + 4.5, "IO0 = кнопка BOOT на плате → «следующий мультик»", 12.5, C["ink"])

# ---------------------------------------------------------------- legend + notes
LX, LY = 900, py(15) + 2
a(f'<rect x="{LX}" y="{LY}" width="{W-40-LX}" height="320" rx="8" fill="#ffffff" stroke="{C["ink"]}" stroke-width="1.4"/>')
text(LX + 18, LY + 28, "Обозначения", 14, C["ink"], "start", "700")
leg = [(C["cath"], "катоды — Down block (GPIO)"), (C["anode"], "аноды — Up block (SPI2)"),
       (C["sd"], "microSD — SPI3"), (C["pwr"], "+3.3V от DevKitC"), (C["gnd"], "GND")]
for i, (col, s) in enumerate(leg):
    yy = LY + 52 + (i % 3) * 22
    lx = LX + 18 + (i // 3) * 270
    a(f'<line x1="{lx}" y1="{yy}" x2="{lx+30}" y2="{yy}" stroke="{col}" stroke-width="3" stroke-linecap="round"/>')
    text(lx + 40, yy + 4.5, s, 12.5)
notes = [
    "1. OE инверсный: жёстко на +3.3V, никогда на GPIO —",
    "   в нуле выходы уходят в Z-state и горят.",
    "2. R1, R2 = 10k с MR на GND: регистры в сбросе,",
    "   пока ESP грузится.",
    "3. Модуль microSD без LDO — питать только от 3.3V.",
    "4. Порядок контактов разъёмов — как на схеме платы;",
    "   перед пайкой сверить с реальной нумерацией.",
    "5. Блоки питаются от 3V3 DevKitC — замерить ток.",
]
ny = LY + 52 + 3 * 22 + 16
text(LX + 18, ny, "Важно", 14, C["ink"], "start", "700")
for i, s in enumerate(notes):
    text(LX + 18, ny + 24 + i * 19, s, 12.2)

# title block
tb_w, tb_h = 330, 46
a(f'<rect x="{W-14-tb_w}" y="{H-14-tb_h}" width="{tb_w}" height="{tb_h}" fill="#ffffff" stroke="{C["ink"]}" stroke-width="1.6"/>')
text(W - 14 - tb_w + 12, H - 14 - 26, "radiolok/igpp32_32 · esp-migration", 12, C["ink"], "start", "700")
text(W - 14 - tb_w + 12, H - 14 - 9, "переходник ESP32-S3 · лист 1 · rev A", 11, C["mute"])

a('</svg>')
open(sys.argv[1], "w").write("\n".join(out))
