"""Tests for tools/igv.py; byte layouts must match the C++ core tests."""
import os
import struct
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(__file__))
import igv  # noqa: E402


def test_header_layout_matches_firmware():
    h = igv.write_header(0x01020304, 40)
    assert len(h) == 32
    assert h[:20] == bytes([ord("I"), ord("G"), ord("V"), ord("1"), 1, 0, 96, 0, 96, 0,
                            0x80, 0x04, 0x04, 0x03, 0x02, 0x01, 40, 0, 1, 0])
    assert h[20:] == bytes(12)
    assert igv.read_header(h) == {"frames": 0x01020304, "frame_ms": 40, "flags": 1}


def test_crc_and_packet_layout():
    assert igv.crc16(b"123456789") == 0x29B1
    p = igv.make_packet(igv.T_PLAY, struct.pack("<H", 3))
    assert p[:4] == b"IGP2"
    assert p[4:7] == bytes([2, 2, 0])
    assert p[7:9] == bytes([3, 0])
    assert struct.unpack("<H", p[9:])[0] == igv.crc16(bytes([2, 2, 0, 3, 0]))


def test_pixel_layout_matches_core():
    img = Image.new("L", (96, 96), 0)
    img.putpixel((0, 0), 255)
    img.putpixel((1, 9), 255)
    img.putpixel((95, 95), 255)
    f = igv.pack_frame(img, threshold=128)
    assert len(f) == 1152
    assert f[0] == 0x7F       # (0,0) lit -> MSB of byte 0 cleared
    assert f[13] == 0xBF      # (1,9): column 1, byte 1, bit 6
    assert f[1151] == 0xFE    # (95,95)
    assert f[1] == 0xFF
    rows = igv.unpack_frame(f)
    assert rows[0][0] and rows[9][1] and rows[95][95] and not rows[8][1]


def test_compatible_with_igpp_py(tmp_path):
    """Same bytes as igpp.py ConvertImage() for a 96x96 picture (no resize)."""
    sys.modules.setdefault("serial", type(sys)("serial"))  # igpp.py imports pyserial
    sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
    import igpp  # noqa: E402

    img = Image.new("L", (96, 96))
    img.putdata([(x * 7 + y * 13) % 256 for y in range(96) for x in range(96)])
    path = tmp_path / "t.png"
    img.save(path)
    ref = bytes(igpp.ConvertImage(str(path), inversed=False))
    ours = igv.pack_frame(Image.open(path), lit="dark", threshold=128)
    assert ours == ref


def test_igv_roundtrip(tmp_path):
    frames = [igv.pack_frame(Image.new("L", (96, 96), v), threshold=128) for v in (0, 255)]
    path = tmp_path / "a.igv"
    igv.write_igv(path, frames, 33)
    hdr, back = igv.read_igv(path)
    assert hdr["frames"] == 2 and hdr["frame_ms"] == 33
    assert back == frames
    assert back[0] == b"\xff" * 1152 and back[1] == b"\x00" * 1152


def test_convert_cli_gif(tmp_path):
    imgs = [Image.new("L", (48, 48), v) for v in (0, 128, 255)]
    gif = tmp_path / "a.gif"
    imgs[0].save(gif, save_all=True, append_images=imgs[1:], duration=100, loop=0)
    out = tmp_path / "a.igv"
    igv.main(["convert", str(gif), "-o", str(out)])
    hdr, frames = igv.read_igv(out)
    assert hdr["frames"] == 3
    assert hdr["frame_ms"] == 100
