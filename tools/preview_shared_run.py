"""Renderiza motions HOTD2 con la matematica exacta del motor GoldSrc."""
import math
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

MOD = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(MOD / "tools"))
sys.path.insert(0, r"D:\games\House_of_the_dead_2_ISO\David_analysis\hotd2-re-reference\tools")
from skel_pose import mot_files, frame_at  # noqa: E402
from tex_export import Exe  # noqa: E402

PARENTS = [-1, 0, 1, 1, 3, 4, 1, 6, 7, 0, 9, 10, 11, 9, 13, 14]
LENS = np.array([0, 10, 8, 8, 7, 6, 8, 7, 6, 2, 12, 11, 4, 12, 11, 4], float)
BASE = MOD / "assets-source/hotd2-randy"
SRC = BASE / "source"
BANKS = {"szom.bin": SRC / "F12585_szom.bin", "zom.bin": SRC / "F12583_zom.bin",
         "ans.bin": SRC / "F12632_ans.bin"}


def angle_quat(ax, ay, az):
    sr, sp, sy = math.sin(ax * .5), math.sin(ay * .5), math.sin(az * .5)
    cr, cp, cy = math.cos(ax * .5), math.cos(ay * .5), math.cos(az * .5)
    return np.array([sr * cp * cy - cr * sp * sy, cr * sp * cy + sr * cp * sy,
                     cr * cp * sy - sr * sp * cy, cr * cp * cy + sr * sp * sy])


def quat_matrix(q):
    x, y, z, w = q
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])


def fk(rot):
    mats, pts = [], [np.zeros(3)] * 16
    for i, (ax, ay, az) in enumerate(rot):
        m = np.eye(4)
        m[:3, :3] = quat_matrix(angle_quat(ax, ay, az))
        m[:3, 3] = np.array([0.0, 0.0, -LENS[i]])
        mats.append(m if PARENTS[i] < 0 else mats[PARENTS[i]] @ m)
        pts[i] = mats[i][:3, 3]
    return np.array(pts)


def load(bank):
    exe = Exe(r"D:\games\House_of_the_dead_2_ISO\HOD2.EXE")
    rows = {name: (n, ids) for n, name, ids in mot_files(exe)}
    _, ids = rows[bank]
    data = BANKS[bank].read_bytes()
    offs = list(struct.unpack_from("<%di" % len(ids), data)) + [len(data)]
    return exe, ids, data, {m: k for k, m in enumerate(ids)}, offs


CACHE = {}


def get(bank, mid):
    if (bank, mid) not in CACHE:
        exe, ids, data, by_id, offs = load(bank)
        k = by_id[mid]
        block = data[offs[k]:offs[k + 1]]
        ticks = exe.s16(0x004E07D0 + 2 * mid)
        nf = (ticks - 1) // 2 + 2
        rots = []
        for f in range(nf):
            _, r = frame_at(block, 16, f)
            rots.append(np.array(r, float) * 2 * math.pi / 65536)
        CACHE[(bank, mid)] = np.array(rots)
    return CACHE[(bank, mid)]


def strip(bank, mid, path, nshow=6):
    rots = get(bank, mid)
    n = len(rots)
    W, H = 170 * nshow, 300
    img = Image.new("RGB", (W, H + 22), (18, 19, 22))
    dr = ImageDraw.Draw(img)
    dr.text((6, 2), "%s 0x%x %dfr (motor GoldSrc)" % (bank, mid, n), fill="white")
    idxs = list(range(0, n, max(1, n // nshow)))[:nshow]
    for t, f in enumerate(idxs):
        pts = fk(rots[f])
        xs, ys = pts[:, 0], pts[:, 1]
        lox, hix = xs.min(), xs.max()
        loy, hiy = ys.min(), 74.0
        s = min(150 / max(1e-3, hix - lox), (H - 16) / max(1e-3, hiy - loy), 7.0)
        ox, oy = t * 170 + 85, H - 12
        for i, par in enumerate(PARENTS):
            if par < 0:
                continue
            dr.line([ox + pts[par][0] * s, oy - pts[par][1] * s,
                     ox + pts[i][0] * s, oy - pts[i][1] * s], fill=(150, 200, 255), width=3)
        for p in pts:
            x0, y0 = int(ox + p[0] * s - 3), int(oy - p[1] * s - 3)
            dr.ellipse([x0, y0, x0 + 6, y0 + 6], fill=(255, 255, 255))
        dr.text((t * 170 + 6, 2), "f%d" % f, fill="white")
    img.save(path)
    print("ok", path)


if __name__ == "__main__":
    out = BASE / "zcand2"
    for bank, mid in [("szom.bin", 0x11F), ("szom.bin", 0x131), ("szom.bin", 0x137),
                      ("zom.bin", 0x3BD), ("zom.bin", 0x3BE), ("zom.bin", 0x3FC),
                      ("zom.bin", 0x3FE), ("zom.bin", 0x420), ("zom.bin", 0x401),
                      ("ans.bin", 0x6), ("ans.bin", 0x0)]:
        strip(bank, mid, out / ("g_%s_%03x.png" % (bank.split(".")[0], mid)))
