#!/usr/bin/env python3
"""Generate docs/img/firmware-logic.svg — how the ESP32-S3 firmware works.

Usage: gen_firmware.py <out.svg>
Sections: boot sequence, tasks on CPU0, double buffer, scan ISR on CPU1,
player modes, and the timing of one scan column.
"""
import sys

W, H = 1700, 1390
FONT = "'JetBrains Mono','DejaVu Sans Mono',Consolas,monospace"
C = dict(
    bg="#f6f3ea", grid="#e6e0d0", ink="#1f2430", mute="#8a8778", box="#ffffff",
    frame="#e8590c", ctrl="#1c7ed6", store="#7048e8", safe="#c92a2a", scan="#0e7c86",
    cpu0="#eef3ff", cpu0e="#748ffc", cpu1="#e6fcf5", cpu1e="#20c997", boot="#495057",
    glow="#ff8a1e", panel="#1d1410", dim="#4a2f1f",
)
out = []
a = out.append


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def text(x, y, s, size=13, fill=None, anchor="start", weight="normal", extra=""):
    a(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill or C["ink"]}" '
      f'text-anchor="{anchor}" font-weight="{weight}" {extra}>{esc(s)}</text>')


def box(x, y, w, h, stroke, title, lines=(), fill=None, tsize=13.5, rx=7, tag=None, center=True):
    a(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill or C["box"]}" '
      f'stroke="{stroke}" stroke-width="2"/>')
    a(f'<rect x="{x}" y="{y}" width="5" height="{h}" rx="2" fill="{stroke}"/>')
    tx = x + w / 2 + 2 if center else x + 16
    anchor = "middle" if center else "start"
    text(tx, y + 22, title, tsize, C["ink"], anchor, "700")
    for i, s in enumerate(lines):
        text(tx, y + 40 + i * 16, s, 11.2, C["mute"], anchor)
    if tag:
        w2 = len(tag) * 6.8 + 12
        a(f'<rect x="{x + w - w2 - 6}" y="{y - 9}" width="{w2}" height="18" rx="9" fill="{stroke}"/>')
        text(x + w - w2 / 2 - 6, y + 4, tag, 10.5, "#ffffff", "middle", "700")


def path(points, color, width=2.2, dash=None, head=True, label=None, lpos=None, lanchor="middle"):
    d = "M" + " L".join(f"{x} {y}" for x, y in points)
    da = f' stroke-dasharray="{dash}"' if dash else ""
    mk = f' marker-end="url(#a-{color[1:]})"' if head else ""
    a(f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{width}" stroke-linejoin="round" '
      f'stroke-linecap="round"{da}{mk}/>')
    if label:
        lx, ly = lpos
        w = len(label) * 6.9 + 12
        bx = lx - w / 2 if lanchor == "middle" else (lx if lanchor == "start" else lx - w)
        a(f'<rect x="{bx}" y="{ly - 9}" width="{w}" height="17" rx="8.5" fill="{C["bg"]}" '
          f'stroke="{color}" stroke-width="1.1"/>')
        text(bx + w / 2, ly + 3.8, label, 10.8, color, "middle", "700")


def zone(x, y, w, h, fill, edge, title, sub):
    a(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="{fill}" stroke="{edge}" '
      f'stroke-width="1.6" stroke-dasharray="6 4"/>')
    text(x + 16, y + 24, title, 15, edge, "start", "700")
    text(x + 16, y + 42, sub, 11.2, C["mute"])


def section(x, y, s):
    text(x, y, s, 15, C["ink"], "start", "700")
    a(f'<line x1="{x}" y1="{y + 7}" x2="{x + len(s) * 9.2}" y2="{y + 7}" stroke="{C["ink"]}" stroke-width="1.4"/>')


# ------------------------------------------------------------------ document
a(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" font-family="{FONT}">')
a('<title>ГИП10000 на стероидах — логика прошивки ESP32-S3</title>')
a('<defs>')
a('<pattern id="grid" width="20" height="20" patternUnits="userSpaceOnUse">'
  f'<path d="M20 0H0V20" fill="none" stroke="{C["grid"]}" stroke-width="0.7"/></pattern>')
for col in sorted({C[k] for k in ("frame", "ctrl", "store", "safe", "scan", "ink", "mute", "boot")}):
    a(f'<marker id="a-{col[1:]}" viewBox="0 0 10 10" refX="8.5" refY="5" markerWidth="7" markerHeight="7" '
      f'orient="auto-start-reverse"><path d="M0 0 L10 5 L0 10 z" fill="{col}"/></marker>')
a('<pattern id="dots" width="6" height="6" patternUnits="userSpaceOnUse">'
  f'<circle cx="3" cy="3" r="1.3" fill="{C["dim"]}"/></pattern>')
a('<pattern id="lit" width="6" height="6" patternUnits="userSpaceOnUse">'
  f'<circle cx="3" cy="3" r="1.6" fill="{C["glow"]}"/></pattern>')
a('<pattern id="hatch" width="5" height="5" patternUnits="userSpaceOnUse" patternTransform="rotate(45)">'
  f'<line x1="0" y1="0" x2="0" y2="5" stroke="{C["scan"]}" stroke-width="2"/></pattern>')
a('</defs>')
a(f'<rect width="{W}" height="{H}" fill="{C["bg"]}"/>')
a(f'<rect width="{W}" height="{H}" fill="url(#grid)" opacity="0.6"/>')
a(f'<rect x="14" y="14" width="{W - 28}" height="{H - 28}" fill="none" stroke="{C["ink"]}" stroke-width="1.6"/>')
text(40, 56, "ГИП10000 на стероидах · логика прошивки ESP32-S3", 24, C["ink"], "start", "700")
text(40, 82, "fw-esp32 · ESP-IDF 5.4 · CPU0 — USB, плеер, носители, сторож · CPU1 — только развёртка в IRAM-прерывании",
     14, C["mute"])

# ------------------------------------------------------------------ boot sequence
section(40, 122, "Запуск")
steps = [
    ("ROM", "загрузчик 1"),
    ("Загрузчик 2", "хук: MR=0, LATCH"),
    ("display::start", "ноги, SPI2, таймер"),
    ("BOOT зажата?", "→ тестовый режим"),
    ("storage::mount", "SD /sd, флеш /flash"),
    ("player::start", "задача плеера"),
    ("usb_link::start", "USB CDC"),
    ("app_main", "статистика 10 с"),
]
bx, by, bw, bh, gap = 40, 138, 190, 54, 13
for i, (t, s) in enumerate(steps):
    x = bx + i * (bw + gap)
    hl = i == 1
    col = C["safe"] if hl else C["boot"]
    pts = f"{x},{by} {x + bw - 14},{by} {x + bw},{by + bh / 2} {x + bw - 14},{by + bh} {x},{by + bh} {x + 14},{by + bh / 2}"
    if i == 0:
        pts = f"{x},{by} {x + bw - 14},{by} {x + bw},{by + bh / 2} {x + bw - 14},{by + bh} {x},{by + bh}"
    a(f'<polygon points="{pts}" fill="{"#fff5f5" if hl else C["box"]}" stroke="{col}" stroke-width="1.8"/>')
    text(x + bw / 2 + 4, by + 22, t, 12.5, col if hl else C["ink"], "middle", "700")
    text(x + bw / 2 + 4, by + 40, s, 10.8, C["mute"], "middle")
text(bx + bw + gap + bw / 2 + 4, by + bh + 18, "через мс после ресета: катоды сброшены", 10.5, C["safe"], "middle", "700")

# ------------------------------------------------------------------ zones
ZY, ZH = 238, 600
IN_X, IN_W = 40, 190
C0_X, C0_W = 260, 770
DB_X, DB_W = 1050, 220
C1_X, C1_W = 1290, 370
zone(C0_X, ZY, C0_W, ZH, C["cpu0"], C["cpu0e"], "CPU0 · задачи FreeRTOS", "всё, что может подождать")
zone(C1_X, ZY, C1_W, ZH, C["cpu1"], C["cpu1e"], "CPU1 · развёртка", "больше на этом ядре ничего нет")
text(DB_X + DB_W / 2, ZY + 24, "двойной буфер", 15, C["frame"], "middle", "700")
text(DB_X + DB_W / 2, ZY + 42, "граница ядер", 11.2, C["mute"], "middle")

# inputs
section(IN_X, ZY + 18, "Входы")
inputs = [
    ("ПК", ["USB CDC, нативный", "igpp.py / igv.py"], C["ctrl"], ZY + 46),
    ("microSD", ["/sd · SPI3", ".igv, папки .bin"], C["store"], ZY + 226),
    ("Флеш FAT", ["/flash · ~13,9 МБ", "запасной носитель"], C["store"], ZY + 318),
    ("Кнопка BOOT", ["GPIO0", "следующий элемент"], C["ctrl"], ZY + 448),
]
for t, ls, col, y in inputs:
    box(IN_X, y, IN_W, 70, col, t, ls)

# --- column A: usb_link, button
UX, UY, UW, UH = 284, ZY + 62, 200, 96
box(UX, UY, UW, UH, C["ctrl"], "usb_link", ["чтение по 512 Б", "StreamParser:", "сырой кадр | IGP2"], tag="prio 6")
path([(IN_X + IN_W, ZY + 72), (UX - 3, ZY + 72)], C["ctrl"])
path([(UX, ZY + 100), (IN_X + IN_W + 3, ZY + 100)], C["ctrl"], dash="5 4", width=1.6, label="ответ",
     lpos=(IN_X + IN_W + 27, ZY + 100))

BTX, BTY, BTW, BTH = 284, ZY + 452, 200, 78
box(BTX, BTY, BTW, BTH, C["ctrl"], "button", ["опрос 10 мс", "антидребезг 30 мс"], tag="prio 4")
path([(IN_X + IN_W, ZY + 483), (BTX - 3, ZY + 483)], C["ctrl"])

# --- column B: commands, player, scan_mon
KX, KY, KW, KH = 560, ZY + 62, 220, 96
box(KX, KY, KW, KH, C["ctrl"], "команды IGP2", ["Play · Next · Stop · Resume", "Status · List · Ping", "File Open / Data / Close"])
path([(UX + UW, UY + 34), (KX - 3, UY + 34)], C["ctrl"], label="пакет", lpos=(UX + UW + 38, UY + 34))

PX, PY, PW, PH = 520, ZY + 238, 300, 150
box(PX, PY, PW, PH, C["frame"], "player", [], tag="prio 5")
items = [("live_frame()", "кадр с USB сразу в submit, режим Live", C["ctrl"]),
         ("плейлист", "*.igv · папки .bin · встроенные узоры", C["ink"]),
         ("Source::read → submit", "темп из .igv, xTaskDelayUntil", C["ink"]),
         ("короткие повторяются", "до 20 с, потом следующий", C["ink"])]
for i, (t_, s_, col) in enumerate(items):
    yy = PY + 44 + i * 26
    text(PX + 18, yy, t_, 11.8, col, "start", "700")
    text(PX + 18, yy + 12, s_, 10.3, C["mute"])

# usb frames -> player (top-left), commands -> player (top)
path([(UX + 170, UY + UH), (UX + 170, PY - 30), (PX + 30, PY - 30), (PX + 30, PY - 3)], C["ctrl"],
     label="кадр", lpos=(UX + 170, UY + UH + 36))
path([(KX + 140, KY + KH), (KX + 140, PY - 3)], C["ctrl"], label="режим / индекс", lpos=(KX + 140, KY + KH + 40))

# storage -> player
path([(IN_X + IN_W, ZY + 261), (250, ZY + 261), (250, PY + 70)], C["store"], head=False)
path([(IN_X + IN_W, ZY + 353), (250, ZY + 353), (250, PY + 70)], C["store"], head=False)
path([(250, PY + 70), (PX - 3, PY + 70)], C["store"], label="чтение", lpos=(395, PY + 70))
a(f'<circle cx="250" cy="{PY + 70}" r="4" fill="{C["store"]}"/>')

# button -> player
path([(BTX + 90, BTY), (BTX + 90, PY + PH + 21), (PX + 40, PY + PH + 21), (PX + 40, PY + PH + 3)], C["ctrl"],
     label="next()", lpos=(BTX + 178, PY + PH + 21))

MX, MY, MW, MH = 520, ZY + 452, 300, 116
box(MX, MY, MW, MH, C["safe"], "scan_mon · сторож", ["каждые 5 мс: растёт ли счётчик кадров?", "нет 50 мс → emergency_off():",
    "флаг стоп, пауза на столбец,", "MR катодов=0 + LATCH → esp_restart()"], fill="#fff5f5", tag="prio max-2")

# --- column C: upload, submit, app_main
SX, SY, SW, SH = 830, ZY + 62, 176, 96
box(SX, SY, SW, SH, C["store"], "загрузка файла", ["в /sd, иначе /flash", "fwrite по кускам", "→ rescan плейлиста"])
path([(KX + KW, KY + 70), (SX - 3, KY + 70)], C["store"], label="File*", lpos=(KX + KW + 25, KY + 70))
path([(SX + 22, SY + SH), (SX + 22, PY + 44), (PX + PW + 3, PY + 44)], C["store"], dash="5 4", width=1.6,
     label="rescan", lpos=(SX + 22, SY + SH + 40))

SUBX, SUBY, SUBW, SUBH = 866, ZY + 238, 140, 150
box(SUBX, SUBY, SUBW, SUBH, C["frame"], "submit()", ["мьютекс", "ждать, пока", "pending = 0", "memcpy → back", "pending = 1"])
path([(PX + PW, PY + 110), (SUBX - 3, PY + 110)], C["frame"], width=2.6)

STX, STY, STW, STH = 866, ZY + 452, 140, 116
box(STX, STY, STW, STH, C["boot"], "app_main", ["раз в 10 с:", "частота скана,", "кадры, USB rx", "→ консоль UART"])

# ------------------------------------------------------------------ double buffer
DBY = ZY + 70
for i, (name, lit, note) in enumerate([("back", False, "пишет submit()"), ("front", True, "читает ISR")]):
    y = DBY + i * 200
    x = DB_X + 30
    a(f'<rect x="{x - 4}" y="{y - 4}" width="{168}" height="{128}" rx="6" fill="#0d0806"/>')
    a(f'<rect x="{x}" y="{y}" width="160" height="120" fill="{C["panel"]}"/>')
    a(f'<rect x="{x}" y="{y}" width="160" height="120" fill="url(#dots)"/>')
    if lit:
        a(f'<rect x="{x + 54}" y="{y}" width="12" height="120" fill="url(#lit)"/>')
    text(x + 80, y + 140, f"{name} · 1152 Б", 12.5, C["ink"], "middle", "700")
    text(x + 80, y + 156, note, 10.8, C["mute"], "middle")
path([(SUBX + SUBW, SUBY + 60), (DB_X + 4, SUBY + 60), (DB_X + 4, DBY + 60), (DB_X + 23, DBY + 60)], C["frame"], width=2.6)
# swap arrow
sx = DB_X + 16
path([(sx, DBY + 70), (sx, DBY + 250)], C["frame"], width=2, dash="4 3")
path([(sx, DBY + 250), (sx, DBY + 70)], C["frame"], width=2, dash="4 3")
a(f'<rect x="{DB_X + 40}" y="{DBY + 168}" width="{DB_W - 54}" height="26" rx="13" fill="#fff4e6" stroke="{C["frame"]}" stroke-width="1.3"/>')
text(DB_X + DB_W / 2 + 13, DBY + 185, "pending (atomic)", 11.5, C["frame"], "middle", "700")
text(DB_X + DB_W / 2, DBY + 420, "обмен только на столбце 0 —", 11, C["ink"], "middle", "700")
text(DB_X + DB_W / 2, DBY + 436, "кадр никогда не рвётся", 11, C["ink"], "middle", "700")
text(DB_X + DB_W / 2, DBY + 460, "back свободен для записи,", 10.8, C["mute"], "middle")
text(DB_X + DB_W / 2, DBY + 475, "когда pending сброшен", 10.8, C["mute"], "middle")

# ------------------------------------------------------------------ CPU1
TX, TY, TW, TH = C1_X + 24, ZY + 62, 322, 88
box(TX, TY, TW, TH, C["scan"], "GPTimer · аларм каждые T", ["T = 151,2 мкс (из master, уточнить)", "96 столбцов → кадр ≈ 69 Гц", "работает и при выключенном кэше"], tag="IRAM ISR")
IX, IY, IW, IH = C1_X + 24, ZY + 166, 322, 250
box(IX, IY, IW, IH, C["scan"], "Scanner::step()", [], fill="#f3fffb")
seq = [("1", "MR анодов ↓↑", "сброс сдвигового регистра"),
       ("2", "SPI2: 12 × 0xFF", "гашение, ждём последний бит"),
       ("3", "шаг катода", "столбец 0: MR + DI=1 + такт"),
       ("", "  столбец 0: обмен буферов", ""),
       ("4", "LATCH обоих", "аноды тёмные, новый катод"),
       ("5", "SPI2: 12 байт столбца", "frame_column_for_chain(pos)"),
       ("6", "LATCH обоих", "столбец горит до следующего T")]
yy = IY + 46
for n, t, s in seq:
    if not n:
        text(IX + 46, yy - 4, t.strip(), 10.8, C["frame"], "start", "700")
        yy += 14
        continue
    a(f'<circle cx="{IX + 26}" cy="{yy - 4}" r="10" fill="{C["scan"]}"/>')
    text(IX + 26, yy, n, 11, "#ffffff", "middle", "700")
    text(IX + 46, yy - 2, t, 12, C["ink"], "start", "700")
    text(IX + 46, yy + 12, s, 10.3, C["mute"])
    yy += 33
path([(TX + TW / 2, TY + TH), (TX + TW / 2, IY - 3)], C["scan"], width=2.4)
# front -> ISR
path([(DB_X + 194, DBY + 260), (IX - 3, DBY + 260)], C["frame"], width=2.6)
text((DB_X + 194 + IX) / 2, DBY + 252, "столбец", 10.8, C["frame"], "middle", "700")
HX, HY, HW, HH = C1_X + 24, ZY + 434, 322, 80
box(HX, HY, HW, HH, C["scan"], "HwPort", ["GPIO W1TS / W1TC — одна запись", "SPI2 через регистры, без DMA", "12 байт в буфере W0–W2"])
path([(IX + IW / 2, IY + IH), (IX + IW / 2, HY - 3)], C["scan"], width=2.4)
# outputs
for i, (t, col) in enumerate([("Up block · аноды", C["frame"]), ("Down block · катоды", C["scan"])]):
    ox = HX + i * 166
    a(f'<rect x="{ox}" y="{ZY + 538}" width="156" height="30" rx="6" fill="{C["box"]}" stroke="{col}" stroke-width="2"/>')
    text(ox + 78, ZY + 558, t, 11.2, col, "middle", "700")
    path([(ox + 78, HY + HH), (ox + 78, ZY + 535)], col, width=2.2)

# monitor reads frames, emergency_off touches cathodes
path([(MX + 240, MY + MH), (MX + 240, ZY + ZH + 14), (HX + 166 + 78, ZY + ZH + 14), (HX + 166 + 78, ZY + 571)], C["safe"],
     dash="6 4", width=1.8, label="emergency_off: катоды выкл.", lpos=(DB_X + 110, ZY + ZH + 14))

# ------------------------------------------------------------------ player modes
BY0 = ZY + ZH + 62
section(40, BY0, "Режимы плеера")
modes = {"pl": (150, BY0 + 110, "Playlist", "по кругу"), "live": (430, BY0 + 110, "Live", "кадры с USB"),
         "stop": (290, BY0 + 300, "Stopped", "панель тёмная")}
for k, (cx, cy, t, s) in modes.items():
    col = {"pl": C["frame"], "live": C["ctrl"], "stop": C["boot"]}[k]
    a(f'<ellipse cx="{cx}" cy="{cy}" rx="88" ry="38" fill="{C["box"]}" stroke="{col}" stroke-width="2.2"/>')
    text(cx, cy - 2, t, 14, col, "middle", "700")
    text(cx, cy + 15, s, 10.8, C["mute"], "middle")
# transitions
path([(238, BY0 + 98), (342, BY0 + 98)], C["ctrl"], label="кадр с USB", lpos=(290, BY0 + 80))
path([(342, BY0 + 124), (238, BY0 + 124)], C["frame"], label="3 с без кадров", lpos=(290, BY0 + 142))
path([(160, BY0 + 148), (230, BY0 + 266)], C["boot"], label="Stop", lpos=(180, BY0 + 207), lanchor="end")
path([(262, BY0 + 264), (190, BY0 + 150)], C["frame"], label="Resume / Play / Next", lpos=(234, BY0 + 200), lanchor="start")
path([(420, BY0 + 148), (350, BY0 + 266)], C["boot"], label="Stop", lpos=(420, BY0 + 215))
text(40, BY0 + 370, "BOOT при включении — тестовый режим: только встроенные узоры", 11, C["mute"])

# ------------------------------------------------------------------ column timing
TX0 = 600
section(TX0, BY0, "Один столбец (не в масштабе)")
rows = ["ANODE_MR", "SPI2 SCK", "CATH_SRCLK", "LATCH (оба)", "выходы анодов", "выход катода"]
x0, x1 = TX0 + 150, W - 60
rh, ry0 = 40, BY0 + 40
# event times
t = {"mr": 0.03, "spi1a": 0.07, "spi1b": 0.24, "clk": 0.28, "l1": 0.33, "spi2a": 0.37, "spi2b": 0.54, "l2": 0.58, "next": 0.97}


def X(f):
    return x0 + (x1 - x0) * f


for i, name in enumerate(rows):
    y = ry0 + i * rh
    text(x0 - 12, y + 24, name, 11.5, C["ink"], "end", "700")
    a(f'<line x1="{x0}" y1="{y + 34}" x2="{x1}" y2="{y + 34}" stroke="{C["grid"]}" stroke-width="1"/>')


def wave(i, segs, color):
    """segs: list of (f, level) changes; level 0/1."""
    y = ry0 + i * rh
    hi, lo = y + 8, y + 30
    pts = []
    for k, (f, lvl) in enumerate(segs):
        yv = hi if lvl else lo
        if k == 0:
            pts.append((X(f), yv))
        else:
            pts.append((X(f), pts[-1][1]))
            pts.append((X(f), yv))
    pts.append((X(1.0), pts[-1][1]))
    a(f'<polyline points="{" ".join(f"{px:.1f},{py}" for px, py in pts)}" fill="none" stroke="{color}" stroke-width="2.2"/>')


pw = 0.012
wave(0, [(0, 1), (t["mr"], 0), (t["mr"] + pw, 1), (t["next"], 0), (t["next"] + pw, 1)], C["frame"])
# SPI bursts
for i_, (fa, fb, lbl) in enumerate([(t["spi1a"], t["spi1b"], "12 × 0xFF"), (t["spi2a"], t["spi2b"], "12 байт столбца")]):
    y = ry0 + rh
    a(f'<line x1="{x0}" y1="{y + 30}" x2="{X(fa)}" y2="{y + 30}" stroke="{C["scan"]}" stroke-width="2.2"/>')
    a(f'<rect x="{X(fa)}" y="{y + 8}" width="{X(fb) - X(fa)}" height="22" fill="url(#hatch)" stroke="{C["scan"]}" stroke-width="1.6"/>')
    text((X(fa) + X(fb)) / 2, y + 4, lbl, 10.5, C["scan"], "middle", "700")
a(f'<line x1="{X(t["spi1b"])}" y1="{ry0 + rh + 30}" x2="{X(t["spi2a"])}" y2="{ry0 + rh + 30}" stroke="{C["scan"]}" stroke-width="2.2"/>')
a(f'<line x1="{X(t["spi2b"])}" y1="{ry0 + rh + 30}" x2="{X(1)}" y2="{ry0 + rh + 30}" stroke="{C["scan"]}" stroke-width="2.2"/>')
wave(2, [(0, 0), (t["clk"], 1), (t["clk"] + pw, 0)], C["scan"])
wave(3, [(0, 0), (t["l1"], 1), (t["l1"] + pw, 0), (t["l2"], 1), (t["l2"] + pw, 0)], C["ink"])
# output bars


def bar(i, fa, fb, label, fill, tc="#ffffff"):
    y = ry0 + i * rh + 7
    a(f'<rect x="{X(fa)}" y="{y}" width="{X(fb) - X(fa)}" height="24" rx="4" fill="{fill}"/>')
    text((X(fa) + X(fb)) / 2, y + 16, label, 10.8, tc, "middle", "700")


bar(4, 0, t["l1"], "столбец N−1", C["frame"])
bar(4, t["l1"], t["l2"], "погашено", "#dee2e6", C["ink"])
bar(4, t["l2"], 1.0, "столбец N горит", C["frame"])
bar(5, 0, t["l1"], "катод N−1", C["scan"])
bar(5, t["l1"], 1.0, "катод N", C["scan"])
# markers
ylo = ry0 + len(rows) * rh + 4
for key, lbl in [("mr", "1"), ("spi1a", "2"), ("clk", "3"), ("l1", "4"), ("spi2a", "5"), ("l2", "6")]:
    xm = X(t[key])
    a(f'<line x1="{xm}" y1="{ry0 - 4}" x2="{xm}" y2="{ylo}" stroke="{C["mute"]}" stroke-width="1" stroke-dasharray="3 3"/>')
    a(f'<circle cx="{xm}" cy="{ylo + 12}" r="9" fill="{C["scan"]}"/>')
    text(xm, ylo + 16, lbl, 10.5, "#ffffff", "middle", "700")
xn = X(t["next"])
a(f'<line x1="{xn}" y1="{ry0 - 4}" x2="{xn}" y2="{ylo}" stroke="{C["ink"]}" stroke-width="1.4" stroke-dasharray="5 3"/>')
text(xn, ylo + 16, "следующий T", 10.5, C["ink"], "middle", "700")
# period brace
yb = ylo + 38
path([(X(t["mr"]), yb), (xn - 2, yb)], C["ink"], width=1.4)
path([(xn - 2, yb), (X(t["mr"]) + 2, yb)], C["ink"], width=1.4)
a(f'<rect x="{(X(t["mr"]) + xn) / 2 - 150}" y="{yb - 10}" width="300" height="20" fill="{C["bg"]}"/>')
text((X(t["mr"]) + xn) / 2, yb + 4, "T — период столбца, тайминги как в MSP430", 11.5, C["ink"], "middle", "700")
text(TX0, yb + 34, "Шаги 1–6 = Scanner::step(). Каждая передача SPI ждёт выхода последнего бита, как UCBUSY на MSP430.", 11, C["mute"])
text(TX0, yb + 52, "Регистры 74HC595: MR чистит сдвиговый регистр, выходы меняются только по LATCH.", 11, C["mute"])

# legend
LGX, LGY = 40, H - 44
leg = [(C["frame"], "кадры"), (C["ctrl"], "управление"), (C["store"], "носители"), (C["scan"], "развёртка"),
       (C["safe"], "защита")]
for i, (col, s) in enumerate(leg):
    x = LGX + i * 170
    a(f'<line x1="{x}" y1="{LGY}" x2="{x + 30}" y2="{LGY}" stroke="{col}" stroke-width="3" stroke-linecap="round"/>')
    text(x + 40, LGY + 4.5, s, 12)

# title block
tb_w, tb_h = 330, 46
a(f'<rect x="{W - 14 - tb_w}" y="{H - 14 - tb_h}" width="{tb_w}" height="{tb_h}" fill="#ffffff" stroke="{C["ink"]}" stroke-width="1.6"/>')
text(W - 14 - tb_w + 12, H - 14 - 26, "radiolok/igpp32_32 · esp-migration", 12, C["ink"], "start", "700")
text(W - 14 - tb_w + 12, H - 14 - 9, "логика прошивки · лист 3 · rev A", 11, C["mute"])
a('</svg>')
open(sys.argv[1], "w").write("\n".join(out))
