#!/usr/bin/env python3
"""Tools for the ГИП10000 96x96 gas-discharge display (ESP32 firmware).

  igv.py convert  INPUT... -o anim.igv [--fps 25] [--threshold N] [--lit dark]
  igv.py info     anim.igv
  igv.py preview  anim.igv -o preview.gif
  igv.py send     anim.igv --port /dev/ttyACM0 [--legacy]   stream live over USB
  igv.py upload   anim.igv --port /dev/ttyACM0               store on SD/flash
  igv.py status|list|next|stop|resume --port /dev/ttyACM0
  igv.py play N   --port /dev/ttyACM0

INPUT can be images, a folder of images (sorted by name), an animated GIF or a
video (mp4/mkv/avi/mov/webm, needs ffmpeg in PATH).

Frame layout matches igpp.py and the firmware: column-major, 12 bytes per
column, MSB = top row, bit 0 = lit dot. By default bright pixels are lit
(white = glowing); --lit dark gives the igpp.py behaviour without --inv.
Floyd-Steinberg dithering is used unless --threshold is given.
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import time

from PIL import Image

W = H = 96
COL_BYTES = 12
FRAME_BYTES = W * COL_BYTES  # 1152
HEADER_SIZE = 32
MAGIC = b"IGV1"
FLAG_LOOP = 1

PKT_MAGIC = b"IGP2"
T_FRAME, T_PLAY, T_STOP, T_RESUME, T_STATUS, T_PING, T_LIST = 1, 2, 3, 4, 5, 6, 7
T_FILE_OPEN, T_FILE_DATA, T_FILE_CLOSE = 0x10, 0x11, 0x12
REPLY = 0x80
VIDEO_EXT = {".mp4", ".mkv", ".avi", ".mov", ".webm", ".m4v"}
IMAGE_EXT = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp", ".tif", ".tiff"}


# ---------------------------------------------------------------- frame format
def pack_frame(img, lit="bright", threshold=None):
    """PIL image -> 1152-byte frame. The image is resized to 96x96."""
    if img.size != (W, H):
        img = img.resize((W, H))
    gray = img.convert("L")
    if threshold is None:
        mono = gray.convert("1", dither=Image.FLOYDSTEINBERG)
        on = [v != 0 for v in mono.getdata()]
    else:
        on = [v >= threshold for v in gray.getdata()]
    if lit == "dark":
        on = [not v for v in on]
    out = bytearray(FRAME_BYTES)
    for x in range(W):
        for yb in range(COL_BYTES):
            b = 0
            for k in range(8):
                y = yb * 8 + k
                if not on[y * W + x]:  # dark dot -> bit 1
                    b |= 0x80 >> k
            out[x * COL_BYTES + yb] = b
    return bytes(out)


def unpack_frame(data):
    """1152-byte frame -> list of rows of booleans (True = lit)."""
    return [[not (data[x * COL_BYTES + (y >> 3)] >> (7 - (y & 7))) & 1 for x in range(W)]
            for y in range(H)]


# ---------------------------------------------------------------- .igv
def write_header(frame_count, frame_ms, flags=FLAG_LOOP):
    head = MAGIC + struct.pack("<HHHHIHH", 1, W, H, FRAME_BYTES, frame_count, frame_ms, flags)
    return head + bytes(HEADER_SIZE - len(head))


def read_header(data):
    if len(data) < HEADER_SIZE or data[:4] != MAGIC:
        raise ValueError("not an IGV1 file")
    version, w, h, fsize, count, ms, flags = struct.unpack_from("<HHHHIHH", data, 4)
    if version != 1 or (w, h, fsize) != (W, H, FRAME_BYTES):
        raise ValueError(f"unsupported file: v{version} {w}x{h} frame {fsize}")
    return {"frames": count, "frame_ms": ms, "flags": flags}


def write_igv(path, frames, frame_ms, loop=True):
    with open(path, "wb") as f:
        f.write(write_header(len(frames), frame_ms, FLAG_LOOP if loop else 0))
        for fr in frames:
            assert len(fr) == FRAME_BYTES
            f.write(fr)


def read_igv(path):
    data = open(path, "rb").read()
    hdr = read_header(data)
    frames = [data[HEADER_SIZE + i * FRAME_BYTES:HEADER_SIZE + (i + 1) * FRAME_BYTES]
              for i in range(hdr["frames"])]
    return hdr, frames


# ---------------------------------------------------------------- inputs
def iter_images(inputs, fps):
    """Yield (PIL image, duration_ms) from files, folders, GIFs and videos."""
    for inp in inputs:
        if os.path.isdir(inp):
            names = sorted(n for n in os.listdir(inp)
                           if os.path.splitext(n)[1].lower() in IMAGE_EXT)
            for n in names:
                yield Image.open(os.path.join(inp, n)), None
            continue
        ext = os.path.splitext(inp)[1].lower()
        if ext in VIDEO_EXT:
            yield from iter_video(inp, fps)
            continue
        img = Image.open(inp)
        if getattr(img, "is_animated", False):
            for i in range(img.n_frames):
                img.seek(i)
                yield img.convert("RGB"), img.info.get("duration")
        else:
            yield img, None


def iter_video(path, fps):
    if not shutil.which("ffmpeg"):
        sys.exit("ffmpeg is needed for video input")
    vf = f"fps={fps},scale={W}:{H}:force_original_aspect_ratio=decrease," \
         f"pad={W}:{H}:(ow-iw)/2:(oh-ih)/2"
    proc = subprocess.Popen(["ffmpeg", "-v", "error", "-i", path, "-vf", vf,
                             "-f", "rawvideo", "-pix_fmt", "gray", "-"], stdout=subprocess.PIPE)
    while True:
        raw = proc.stdout.read(W * H)
        if len(raw) < W * H:
            break
        yield Image.frombytes("L", (W, H), raw), None
    proc.wait()


# ---------------------------------------------------------------- serial link
def crc16(data, crc=0xFFFF):
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def make_packet(ptype, payload=b""):
    head = struct.pack("<BH", ptype, len(payload))
    return PKT_MAGIC + head + payload + struct.pack("<H", crc16(head + payload))


def open_port(port):
    import serial  # pyserial
    return serial.Serial(port, 115200, timeout=0.5)


def read_reply(ser, timeout=2.0):
    """Read one reply packet: returns (type, payload) or None."""
    deadline = time.time() + timeout
    buf = b""
    while time.time() < deadline:
        buf += ser.read(ser.in_waiting or 1)
        i = buf.find(PKT_MAGIC)
        if i < 0 or len(buf) < i + 9:
            continue
        ptype, ln = struct.unpack_from("<BH", buf, i + 4)
        end = i + 7 + ln + 2
        if len(buf) < end:
            continue
        payload = buf[i + 7:i + 7 + ln]
        crc, = struct.unpack_from("<H", buf, i + 7 + ln)
        if crc != crc16(buf[i + 4:i + 7] + payload):
            buf = buf[i + 1:]
            continue
        return ptype & ~REPLY, payload
    return None


# ---------------------------------------------------------------- commands
def cmd_convert(a):
    frames, durations = [], []
    for img, dur in iter_images(a.inputs, a.fps):
        frames.append(pack_frame(img, a.lit, a.threshold))
        durations.append(dur)
    if not frames:
        sys.exit("no frames")
    gif_ms = [d for d in durations if d]
    frame_ms = int(round(sum(gif_ms) / len(gif_ms))) if gif_ms and not a.force_fps else int(round(1000 / a.fps))
    write_igv(a.output, frames, max(1, frame_ms), loop=not a.once)
    print(f"{a.output}: {len(frames)} frames, {frame_ms} ms/frame, "
          f"{os.path.getsize(a.output) / 1024:.0f} KiB")


def cmd_info(a):
    hdr, frames = read_igv(a.file)
    lit = sum(sum(r.count(True) for r in unpack_frame(f)) for f in frames) / max(1, len(frames))
    print(f"{a.file}: {hdr['frames']} frames, {hdr['frame_ms']} ms/frame "
          f"({1000 / hdr['frame_ms']:.1f} fps), loop={bool(hdr['flags'] & FLAG_LOOP)}, "
          f"avg lit {lit:.0f}/{W * H} dots")


def render_preview(frame, scale=5):
    img = Image.new("RGB", (W * scale, H * scale), (29, 20, 16))
    px = img.load()
    rows = unpack_frame(frame)
    r = max(1, scale // 2 - 1)
    for y in range(H):
        for x in range(W):
            col = (255, 138, 30) if rows[y][x] else (64, 40, 26)
            cx, cy = x * scale + scale // 2, y * scale + scale // 2
            for dy in range(-r, r + 1):
                for dx in range(-r, r + 1):
                    if dx * dx + dy * dy <= r * r:
                        px[cx + dx, cy + dy] = col
    return img


def cmd_preview(a):
    hdr, frames = read_igv(a.file)
    imgs = [render_preview(f) for f in frames[:a.max_frames]]
    if a.output.lower().endswith(".gif") and len(imgs) > 1:
        imgs[0].save(a.output, save_all=True, append_images=imgs[1:], duration=hdr["frame_ms"], loop=0)
    else:
        imgs[0].save(a.output)
    print(f"{a.output}: {len(imgs)} frames")


def cmd_send(a):
    hdr, frames = read_igv(a.file)
    ser = open_port(a.port)
    period = hdr["frame_ms"] / 1000.0
    t = time.time()
    n = 0
    try:
        while True:
            for f in frames:
                ser.write(f if a.legacy else make_packet(T_FRAME, f))
                n += 1
                t += period
                time.sleep(max(0.0, t - time.time()))
            if not a.loop:
                break
    except KeyboardInterrupt:
        pass
    print(f"sent {n} frames")


def cmd_upload(a):
    data = open(a.file, "rb").read()
    read_header(data)
    name = a.name or os.path.basename(a.file)
    ser = open_port(a.port)
    ser.write(make_packet(T_FILE_OPEN, name.encode()))
    r = read_reply(ser)
    if not r or r[1][:1] != b"\x00":
        sys.exit(f"device refused the file: {r}")
    chunk = 1024
    t0 = time.time()
    for i in range(0, len(data), chunk):
        ser.write(make_packet(T_FILE_DATA, data[i:i + chunk]))
        if i % (chunk * 64) == 0:
            print(f"\r{i * 100 // len(data)}%", end="", flush=True)
    ser.write(make_packet(T_FILE_CLOSE))
    r = read_reply(ser, timeout=10)
    if not r or r[1][:1] != b"\x00":
        sys.exit(f"\nupload failed: {r}")
    size, = struct.unpack_from("<I", r[1], 1)
    print(f"\r{name}: {size} bytes in {time.time() - t0:.1f} s")


def cmd_simple(a):
    ser = open_port(a.port)
    ptype = {"status": T_STATUS, "list": T_LIST, "next": T_PLAY, "stop": T_STOP,
             "resume": T_RESUME, "play": T_PLAY}[a.cmd]
    payload = b""
    if a.cmd == "next":
        payload = struct.pack("<H", 0xFFFF)
    elif a.cmd == "play":
        payload = struct.pack("<H", a.index)
    ser.write(make_packet(ptype, payload))
    r = read_reply(ser)
    if r is None:
        sys.exit("no reply")
    if a.cmd == "status":
        f = struct.unpack("<IIIIHHBB", r[1])
        keys = ["scan_frames", "shown_frames", "rx_frames", "rx_dropped", "playlist_size",
                "playlist_index", "mode", "storage"]
        for k, v in zip(keys, f):
            print(f"{k:15} {v}")
    elif a.cmd == "list":
        for i, n in enumerate(r[1].decode(errors="replace").splitlines()):
            print(f"{i:3}  {n}")
    else:
        print("ok" if r[1][:1] == b"\x00" else f"error {r[1].hex()}")


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)

    c = sub.add_parser("convert", help="images/GIF/video -> .igv")
    c.add_argument("inputs", nargs="+")
    c.add_argument("-o", "--output", required=True)
    c.add_argument("--fps", type=float, default=25.0)
    c.add_argument("--force-fps", action="store_true", help="ignore GIF frame durations")
    c.add_argument("--threshold", type=int, help="threshold 0..255 instead of dithering")
    c.add_argument("--lit", choices=["bright", "dark"], default="bright")
    c.add_argument("--once", action="store_true", help="clear the loop flag")
    c.set_defaults(fn=cmd_convert)

    c = sub.add_parser("info")
    c.add_argument("file")
    c.set_defaults(fn=cmd_info)

    c = sub.add_parser("preview")
    c.add_argument("file")
    c.add_argument("-o", "--output", required=True)
    c.add_argument("--max-frames", type=int, default=500)
    c.set_defaults(fn=cmd_preview)

    c = sub.add_parser("send", help="stream frames live")
    c.add_argument("file")
    c.add_argument("--port", required=True)
    c.add_argument("--legacy", action="store_true", help="raw 1152-byte frames like igpp.py")
    c.add_argument("--loop", action="store_true")
    c.set_defaults(fn=cmd_send)

    c = sub.add_parser("upload", help="store a .igv on the device")
    c.add_argument("file")
    c.add_argument("--port", required=True)
    c.add_argument("--name")
    c.set_defaults(fn=cmd_upload)

    for name in ("status", "list", "next", "stop", "resume"):
        c = sub.add_parser(name)
        c.add_argument("--port", required=True)
        c.set_defaults(fn=cmd_simple)
    c = sub.add_parser("play")
    c.add_argument("index", type=int)
    c.add_argument("--port", required=True)
    c.set_defaults(fn=cmd_simple)

    a = p.parse_args(argv)
    a.fn(a)


if __name__ == "__main__":
    main()
