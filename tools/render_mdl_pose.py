"""Dibuja el esqueleto de una secuencia MDL para comparar poses entre modelos.

Calcula la cinematica directa con la convencion GoldSrc (local = T(pos) * R(rad))
y proyecta los huesos en dos vistas. Sirve para comprobar que un retarget de
animacion conserva las poses del modelo donante.

Uso: py tools/render_mdl_pose.py <salida.png> frame1=modelo1.mdl:seq ... 
  Ej: py tools/render_mdl_pose.py out.png 0=david.mdl:run 8=david.mdl:run
"""
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

NB = 16


def load(path):
    return open(path, "rb").read()


def header(d):
    return (struct.unpack_from("<i", d, 140)[0], struct.unpack_from("<i", d, 144)[0],
            struct.unpack_from("<i", d, 164)[0], struct.unpack_from("<i", d, 168)[0])


def bones(d):
    nb, bo, _, _ = header(d)
    out = []
    for i in range(nb):
        b = bo + i * 112
        name = d[b:b + 32].split(b"\x00")[0].decode()
        parent = struct.unpack_from("<i", d, b + 32)[0]
        pos = struct.unpack_from("<3f", d, b + 64)
        rot = struct.unpack_from("<3f", d, b + 76)
        scale = struct.unpack_from("<6f", d, b + 88)
        out.append((name, parent, np.array(pos), np.array(rot), np.array(scale)))
    return out


def seq(d, label):
    nb, bo, ns, so = header(d)
    for i in range(ns):
        b = so + i * 176
        if d[b:b + 32].split(b"\x00")[0].decode() == label:
            return {"base": b,
                    "frames": struct.unpack_from("<i", d, b + 56)[0],
                    "animindex": struct.unpack_from("<i", d, b + 124)[0]}
    raise KeyError(label)


def expand(d, ai, bone, ch, frames):
    base = ai + bone * 12
    offs = struct.unpack_from("<6H", d, base)
    if offs[ch] == 0:
        return None
    p = base + offs[ch]
    vals, got = [], 0
    while got < frames:
        valid, total = struct.unpack_from("<BB", d, p)
        chunk = struct.unpack_from("<%dh" % valid, d, p + 2)
        for k in range(total):
            vals.append(chunk[k] if k < valid else chunk[valid - 1])
        got += total
        p += 2 + valid * 2
    return vals


def qmatrix(q):
    x, y, z, w = q
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])


def angle_quat(ax, ay, az):
    sy, cy = np.sin(az * .5), np.cos(az * .5)
    sp, cp = np.sin(ay * .5), np.cos(ay * .5)
    sr, cr = np.sin(ax * .5), np.cos(ax * .5)
    return np.array([sr * cp * cy - cr * sp * sy,
                     cr * sp * cy + sr * cp * sy,
                     cr * cp * sy - sr * sp * cy,
                     cr * cp * cy + sr * sp * sy])


def pose(path, label, frame):
    d = load(path)
    bl = bones(d)
    s = seq(d, label)
    ai, nf = s["animindex"], s["frames"]
    local, world = [], []
    for i, (name, parent, base_pos, base_rot, scale) in enumerate(bl):
        vals = expand(d, ai, i, 0, nf)
        if vals:
            pos = np.array([base_pos[c] + expand(d, ai, i, c, nf)[frame] * scale[c]
                            for c in range(3)])
        else:
            pos = base_pos.copy()
        rot = base_rot.astype(float).copy()
        for c in range(3):
            ch = expand(d, ai, i, c + 3, nf)
            if ch:
                rot[c] = base_rot[c] + ch[frame] * scale[3 + c]
        m = np.eye(4)
        m[:3, :3] = qmatrix(angle_quat(*rot))
        m[:3, 3] = pos
        local.append(m)
    for i, (name, parent, _, _, _) in enumerate(bl):
        world.append(local[i] if parent < 0 else world[parent] @ local[i])
    return [w[:3, 3] for w in world], [b[1] for b in bl]


def draw(panels, out):
    W, H = 320, 420
    img = Image.new("RGB", (W * len(panels), H + 26), (18, 19, 22))
    dr = ImageDraw.Draw(img)
    palette = [(240, 200, 70), (90, 200, 255), (255, 120, 140), (140, 255, 160),
               (200, 150, 255), (255, 170, 90), (120, 220, 220), (230, 230, 230)]
    for n, (title, joint, parent) in enumerate(panels):
        pts = np.array(joint)
        # vista lateral (X-Z) y frontal (Y-Z) lado a lado
        for view, (a, b) in enumerate([(0, 2), (1, 2)]):
            sub = pts[:, [a, b]]
            lo = sub.min(axis=0)
            hi = sub.max(axis=0)
            span = max(1e-3, (hi - lo).max())
            ox = n * W + view * W // 2 + W // 4
            oy = H // 2
            scale = (H * .78) / span
            prj = [(ox + (p[0] - (lo[0] + hi[0]) / 2) * scale,
                    oy - (p[1] - (lo[1] + hi[1]) / 2) * scale) for p in sub]
            for i, par in enumerate(parent):
                if par < 0:
                    continue
                dr.line([prj[par], prj[i]], fill=palette[par % len(palette)], width=3)
            for p in prj:
                dr.ellipse([p[0] - 2, p[1] - 2, p[0] + 2, p[1] + 2], fill=(255, 255, 255))
        dr.text((n * W + 8, H + 6), title, fill=(210, 210, 210))
    img.save(out)
    print("escrito:", out)


if __name__ == "__main__":
    out = sys.argv[1]
    panels = []
    for spec in sys.argv[2:]:
        frame, rest = spec.split("=", 1)
        model, label = rest.rsplit(":", 1)
        joint, parent = pose(model, label, int(frame))
        panels.append(("%s %s f%s" % (Path(model).name, label, frame), joint, parent))
    draw(panels, out)
