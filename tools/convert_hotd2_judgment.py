"""Assemble Judgment's Kuarl and Zeal rigs into animated GoldSrc models."""
from __future__ import annotations

import collections
import hashlib
import json
import math
import struct
import subprocess
import sys
import warnings
from pathlib import Path

import numpy as np
from PIL import Image
from scipy.spatial.transform import Rotation

MOD = Path(__file__).resolve().parents[1]
BASE = MOD / "assets-source" / "hotd2-judgment"
SOURCE = BASE / "source"
OUT = BASE / "goldsrc"
DEST = MOD / "models" / "cremation" / "hotd2" / "judgment"
COMPILER = MOD.parent.parent / "Half-Life SDK" / "Model Tools" / "studiomdl.exe"
REF_TOOLS = Path(r"D:\games\House_of_the_dead_2_ISO\David_analysis\hotd2-re-reference\tools")
sys.path.insert(0, str(REF_TOOLS))
from pol_parse import header_count, parse_model, pol_files, read_header, triangles  # noqa: E402
from prs import decompress  # noqa: E402
from skel_pose import frame_at, mot_files, skeleton  # noqa: E402
from tex_export import Exe, decode_texture, descriptors, rgba_bytes, texture_files  # noqa: E402

# HOTD2 faces -Z; Cremation's GoldSrc actors face +X. This is the same proper
# rotation used by the other HOTD2 ports in this mod.
C = np.array([[0, 0, -1], [-1, 0, 0], [0, 1, 0]], dtype=float)
RIGS = {
    "kuarl": {"kind": 0x44, "model_file": "boss1q.bin", "model_source": "F13033_pol_boss1q.bin",
              "texture_file": "boss1q.bin", "texture_source": "F13411_boss1q.bin",
              "motions": tuple(range(0x383, 0x395)), "companion": None, "target_height": 280.0,
              "override": {2: 0}},
    "zeal": {"kind": 0x45, "model_file": "boss1z.bin", "model_source": "F13034_pol_boss1z.bin",
             "texture_file": "boss1z.bin", "texture_source": "F13412_boss1z.bin",
             "motions": tuple(range(0x408, 0x417)), "companion": "wings", "target_height": 56.0,
             "override": {2: 0x2A9}},
}
MOTION_NAMES = {
    0x38D: "standing", 0x393: "walk", 0x394: "fast_walk",
    0x411: "hover", 0x40C: "charge", 0x40A: "death",
    0x408: "hit_reaction_1", 0x409: "hit_reaction_2",
}


def raw_or_prs(path: Path) -> bytes:
    raw = path.read_bytes()
    if len(raw) >= 8:
        wanted = struct.unpack_from("<I", raw)[0]
        try:
            decoded = decompress(raw[4:])
            if len(decoded) == wanted:
                return decoded
        except (ValueError, IndexError):
            pass
    return raw


def pol_models(exe: Exe, data: bytes, filename: str) -> tuple[int, list[int], dict[int, dict]]:
    rows = [(n, ids) for _, n, ids in pol_files(exe) if n == filename and ids]
    if len(rows) != 1:
        raise RuntimeError(f"Expected one populated POL bank {filename}, found {len(rows)}")
    _, ids = rows[0]
    headers = read_header(data, header_count(data))
    if len(headers) - 1 != len(ids):
        raise RuntimeError(f"POL/header count mismatch for {filename}: {len(headers)-1} != {len(ids)}")
    models = {}
    for k, model_id in enumerate(ids):
        hdr, meshes = parse_model(data, headers[k][2], headers[k + 1][0])
        verts, lookup, faces, materials, face_materials = [], {}, [], [], []
        for mesh in meshes:
            mi = len(materials)
            words = list(mesh.material)
            materials.append({"words": words, "texture": words[8]})
            for tri in triangles(mesh):
                coords = np.asarray([v[:3] for v in tri], dtype=np.float64)
                normal = np.cross(coords[1] - coords[0], coords[2] - coords[0])
                if np.linalg.norm(normal) < 1e-10:
                    continue
                if np.dot(normal, np.asarray([v[3:6] for v in tri]).mean(0)) < 0:
                    tri = (tri[0], tri[2], tri[1])
                face = []
                for v in tri:
                    if not all(np.isfinite(x) for x in v):
                        raise RuntimeError(f"Non-finite vertex in model {model_id}")
                    if v not in lookup:
                        lookup[v] = len(verts)
                        verts.append(v)
                    face.append(lookup[v])
                faces.append(face)
                face_materials.append(mi)
        models[model_id] = {"id": model_id, "header": hdr, "vertices": verts, "faces": faces,
                            "materials": materials, "face_materials": face_materials}
    return len(ids), ids, models


def extract_clip(exe: Exe, count: int, motion_id: int, motion_data: bytes, ids: list[int], bank_index: int):
    if motion_id not in ids:
        raise RuntimeError(f"Motion 0x{motion_id:03X} is missing from boss1.bin")
    offsets = list(struct.unpack_from(f"<{len(ids)}I", motion_data)) + [len(motion_data)]
    k = ids.index(motion_id)
    block = motion_data[offsets[k]:offsets[k + 1]]
    ticks = exe.s16(0x004E07D0 + 2 * motion_id)
    frame_count = (ticks - 1) // 2 + 2
    stride = (12 + 6 * count + 3) & ~3
    if len(block) != 4 + frame_count * stride:
        raise RuntimeError(f"Motion 0x{motion_id:03X}: unexpected block length {len(block)}")
    frames = []
    for frame in range(frame_count):
        pos, angles = frame_at(block, count, frame)
        frames.append({"position": list(pos), "rotations": list(angles)})
    return {"id": motion_id, "bank_index": bank_index, "ticks_60hz": ticks, "frames": frames}


def attachment_specs(exe: Exe, kind: int) -> list[tuple[int, int]]:
    """Return the extra model/bone pairs loaded by the original model gallery."""
    at = exe.u32(0x0052ED08 + 4 * kind)
    count, definitions = exe.u32(at), exe.u32(at + 4)
    table = 0x004ED4B0 if kind in (0x1F, 0x46, 0x4E) else 0x004ED1E0
    out = []
    for index in range(count):
        definition = exe.u32(definitions + 4 * index)
        if not definition:
            continue
        model_id = exe.u32(definition)
        bone = exe.u32(table + 0x14 * index + 0x10) & 15
        if model_id:
            out.append((bone, model_id))
    return out


def load_inputs(exe: Exe, entry: dict, rig_name: str):
    cfg = RIGS[rig_name]
    root, count = skeleton(exe, cfg["kind"])
    nodes = []

    def visit(node, parent=None):
        nodes.append({"index": node.index, "model_id": cfg["override"].get(node.index, node.model_id),
                      "source_model_id": node.model_id, "offset": node.offset, "flags": node.flags,
                      "parent": parent})
        for child in node.children:
            visit(child, node.index)

    visit(root)
    if count != len(nodes) or count != 16:
        raise RuntimeError(f"Unexpected {rig_name} rig size: {count}/{len(nodes)}")
    geom_path = SOURCE / cfg["model_source"]
    texture_path = SOURCE / cfg["texture_source"]
    geom_count, model_ids, models = pol_models(exe, raw_or_prs(geom_path), cfg["model_file"])
    active = {n["model_id"] for n in nodes if n["model_id"]}
    missing = active - models.keys()
    if missing:
        raise RuntimeError(f"{rig_name} skeleton references absent meshes: {sorted(missing)}")

    texture_row = next((row for row in texture_files(exe) if row[1] == cfg["texture_file"]), None)
    if texture_row is None:
        raise RuntimeError(f"No texture table for {cfg['texture_file']}")
    _, _, tex_addr, tex_count = texture_row
    bank = raw_or_prs(texture_path)
    textures = {}
    uses = collections.defaultdict(list)
    for node in nodes:
        model_id = node["model_id"]
        if not model_id:
            continue
        for mi, material in enumerate(models[model_id]["materials"]):
            ti = material["texture"]
            if ti != 0xFFFFFFFF:
                uses[ti].append((model_id, mi))
    texture_dir = BASE / "textures"
    texture_dir.mkdir(parents=True, exist_ok=True)
    desc_list = descriptors(exe, tex_addr, tex_count)
    for desc in desc_list:
        width, height, texels = decode_texture(bank, desc)
        rgba = rgba_bytes(texels, desc["fmt"] & 7)
        image_name = f"{rig_name}_tex_{desc['index']:02d}_id{desc['id']}.png"
        image = Image.frombytes("RGBA", (width, height), rgba).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
        image.save(texture_dir / image_name)
        alpha = np.asarray(image)[:, :, 3]
        textures[desc["index"]] = {"path": texture_dir / image_name, "width": width, "height": height,
                                    "alpha_zero": int(np.sum(alpha == 0)),
                                    "alpha_partial": int(np.sum((alpha > 0) & (alpha < 255))),
                                    "material_uses": uses[desc["index"]]}

    bank_rows = [(i, name, ids) for i, name, ids in mot_files(exe) if name == "boss1.bin"]
    if len(bank_rows) != 1:
        raise RuntimeError("Expected one boss1 motion bank")
    bank_index, _, motion_ids = bank_rows[0]
    motion_data = raw_or_prs(SOURCE / "F12629_boss1.bin")
    clips = {mid: extract_clip(exe, count, mid, motion_data, motion_ids, bank_index)
             for mid in cfg["motions"]}
    companion_clips = {}
    companion_nodes = []
    if cfg["companion"]:
        wing_root, wing_count = skeleton(exe, 0x46)

        def visit_wing(node, parent=None):
            companion_nodes.append({"index": node.index, "model_id": node.model_id, "offset": node.offset,
                                    "flags": node.flags, "parent": parent})
            for child in node.children:
                visit_wing(child, node.index)

        visit_wing(wing_root)
        if wing_count != len(companion_nodes) or wing_count != 7:
            raise RuntimeError("Unexpected Zeal wing rig size")
        wing_count_models, _, wing_models = pol_models(exe, raw_or_prs(SOURCE / "F13035_pol_boss1z_wing.bin"),
                                                        "boss1z_wing.bin")
        if wing_count_models != 8:
            raise RuntimeError(f"Unexpected wing model bank size {wing_count_models}")
        companion_active = {n["model_id"] for n in companion_nodes if n["model_id"]}
        if companion_active - wing_models.keys():
            raise RuntimeError("Zeal wing skeleton references absent mesh")
        entry["models"].update(wing_models)
        wing_row = next(row for row in texture_files(exe) if row[1] == "boss1z_wing.bin")
        _, _, wing_tex_addr, wing_tex_count = wing_row
        wing_bank = raw_or_prs(SOURCE / "F13413_boss1z_wing.bin")
        for desc in descriptors(exe, wing_tex_addr, wing_tex_count):
            w, h, texels = decode_texture(wing_bank, desc)
            rgba = rgba_bytes(texels, desc["fmt"] & 7)
            path = texture_dir / f"zeal_wings_tex_{desc['index']:02d}_id{desc['id']}.png"
            im = Image.frombytes("RGBA", (w, h), rgba).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
            im.save(path)
            alpha = np.asarray(im)[:, :, 3]
            textures[100 + desc["index"]] = {"path": path, "width": w, "height": h,
                                              "alpha_zero": int(np.sum(alpha == 0)),
                                              "alpha_partial": int(np.sum((alpha > 0) & (alpha < 255))),
                                              "material_uses": []}
        for mid in cfg["motions"]:
            wing_mid = 0x14 if mid == 0x40C else 0x10
            companion_clips[mid] = extract_clip(exe, 7, wing_mid, motion_data, motion_ids, bank_index)
        return nodes, models, textures, clips, companion_nodes, companion_clips
    return nodes, models, textures, clips, companion_nodes, companion_clips


def indexed_texture(path: Path, name: str, masked: bool, mirror_u: bool, mirror_v: bool, diffuse: tuple[float, ...]):
    image = Image.open(path).convert("RGBA")
    rgba = np.asarray(image)
    rgb = np.clip(np.round(rgba[:, :, :3].astype(float) * np.asarray(diffuse)), 0, 255).astype(np.uint8)
    alpha = rgba[:, :, 3] if masked else np.full(rgba.shape[:2], 255, dtype=np.uint8)
    if mirror_u:
        rgb = np.concatenate([rgb, rgb[:, ::-1]], axis=1)
        alpha = np.concatenate([alpha, alpha[:, ::-1]], axis=1)
    if mirror_v:
        rgb = np.concatenate([rgb[::-1], rgb], axis=0)
        alpha = np.concatenate([alpha[::-1], alpha], axis=0)
    rgb_image = Image.fromarray(rgb)
    if masked and np.any(alpha < 128):
        quant = rgb_image.quantize(colors=255)
        palette = quant.getpalette()[:765] + [0, 0, 255]
        pixels = np.asarray(quant).copy()
        pixels[alpha < 128] = 255
        quant = Image.fromarray(pixels, "P")
        quant.putpalette(palette)
        out_name = "{" + name
    else:
        quant = rgb_image.quantize(colors=256)
        out_name = name
    quant.save(OUT / out_name)
    return out_name


def build_one(exe: Exe, rig_name: str):
    global OUT
    cfg = RIGS[rig_name]
    DEST.mkdir(parents=True, exist_ok=True)
    out = OUT / rig_name
    out.mkdir(parents=True, exist_ok=True)
    old_out = OUT
    OUT = out
    try:
        holder = {"models": {}}
        nodes, models, textures, clips, companion_nodes, companion_clips = load_inputs(exe, holder, rig_name)
        models.update(holder["models"])
        parents = [-1 if n["parent"] is None else n["parent"] for n in nodes]
        bone_names = ["Root", "Body", "Head", "Arm_R", "Forearm_R", "Hand_R", "Arm_L", "Forearm_L",
                      "Hand_L", "Pelvis", "Thigh_R", "Shin_R", "Foot_R", "Thigh_L", "Shin_L", "Foot_L"]
        offsets = np.array([n["offset"] for n in nodes], dtype=float) @ C.T
        joined_parents = parents.copy()
        all_offsets = list(offsets)
        all_names = bone_names.copy()
        wing_bone_map = {}
        if companion_nodes:
            for n in companion_nodes:
                idx = 16 + n["index"]
                wing_bone_map[n["index"]] = idx
                parent = 1 if n["parent"] is None else wing_bone_map[n["parent"]]
                joined_parents.append(parent)
                all_offsets.append(np.asarray(n["offset"], dtype=float) @ C.T)
                all_names.append("WingRoot" if n["index"] == 0 else f"Wing_{n['index']}")
        all_offsets = np.asarray(all_offsets, dtype=float)
        rest = all_offsets.copy()
        for i in range(1, len(rest)):
            rest[i] = rest[joined_parents[i]] + rest[i]

        ids_for_nodes = [(n["index"], n["model_id"]) for n in nodes if n["model_id"]]
        attachments = [(bone, model_id) for bone, model_id in attachment_specs(exe, cfg["kind"])
                       if model_id in models]
        ids_for_nodes += attachments
        if companion_nodes:
            ids_for_nodes += [(wing_bone_map[n["index"]], n["model_id"])
                              for n in companion_nodes if n["model_id"]]
            ids_for_nodes += [(wing_bone_map[bone], model_id)
                              for bone, model_id in attachment_specs(exe, 0x46)
                              if model_id in models]
        points = []
        for bone, model_id in ids_for_nodes:
            verts = np.asarray(models[model_id]["vertices"], dtype=float)
            if len(verts):
                points.extend(verts[:, :3] @ C.T + rest[bone])
        points = np.asarray(points)
        height = float(points[:, 2].max() - points[:, 2].min())
        if height <= 0:
            raise RuntimeError(f"Invalid {rig_name} mesh bounds")
        scale = cfg["target_height"] / height
        floor = -float(points[:, 2].min()) * scale
        local_offsets = all_offsets * scale
        local_offsets[0] = [0, 0, floor]
        rest = local_offsets.copy()
        for i in range(1, len(rest)):
            rest[i] = rest[joined_parents[i]] + rest[i]

        # Build a unique, indexed 8-bit GoldSrc texture for each material state.
        texture_names, uv_div = {}, {}
        for bone, model_id in ids_for_nodes:
            model = models[model_id]
            for mi, mat in enumerate(model["materials"]):
                words = mat["words"]
                ti = words[8]
                diffuse = tuple(struct.unpack("<f", struct.pack("<I", v))[0] for v in words[12:15])
                tsp = words[2]
                masked = (tsp & 0x180000) != 0x80000
                mirror_u = bool(tsp & 0x40000) and not bool(tsp & 0x10000)
                mirror_v = bool(tsp & 0x20000) and not bool(tsp & 0x8000)
                tex_key = (100 + ti if bone >= 16 else ti)
                key = (tex_key, diffuse, masked, mirror_u, mirror_v)
                if key not in texture_names:
                    if tex_key not in textures:
                        image = Image.new("RGBA", (16, 16), (255, 255, 255, 255))
                        missing_path = out / f"missing_{tex_key}.png"
                        image.save(missing_path)
                        textures[tex_key] = {"path": missing_path, "alpha_zero": 0, "alpha_partial": 0}
                    tex = textures[tex_key]
                    filename = f"{rig_name}_{len(texture_names):03d}.bmp"
                    texture_names[key] = indexed_texture(tex["path"], filename, masked and tex["alpha_zero"] > 0,
                                                         mirror_u, mirror_v, diffuse)
                texture_names[(model_id, mi)] = texture_names[key]
                uv_div[(model_id, mi)] = (2 if mirror_u else 1, 2 if mirror_v else 1)

        nodes_text = "version 1\nnodes\n" + "".join(
            f'{i} "{name}" {joined_parents[i]}\n' for i, name in enumerate(all_names)) + "end\n"
        ref = nodes_text + "skeleton\ntime 0\n" + "".join(
            f"{i} " + " ".join(f"{x:.7f}" for x in (*local_offsets[i], 0, 0, 0)) + "\n"
            for i in range(len(all_names))) + "end\n"
        triangles_text = []
        for bone, model_id in ids_for_nodes:
            model = models[model_id]
            verts = np.asarray(model["vertices"], dtype=float)
            mesh_pos = verts[:, :3] @ C.T * scale + rest[bone]
            mesh_norm = verts[:, 3:6] @ C.T
            for face, mi in zip(model["faces"], model["face_materials"]):
                triangles_text.append(texture_names[(model_id, mi)])
                du, dv = uv_div[(model_id, mi)]
                for vi in face:
                    v = verts[vi]
                    uv = v[6:8]
                    triangles_text.append(str(bone) + " " + " ".join(f"{x:.7f}" for x in
                        (*mesh_pos[vi], *mesh_norm[vi], uv[0] / du, uv[1] / dv)))
        (out / f"{rig_name}_mesh.smd").write_text(ref + "triangles\n" +
            "\n".join(triangles_text) + "\nend\n", encoding="ascii")

        # Animate every source Kuarl/Zeal clip; Zeal's independent wing rig is
        # synchronized per frame and attached directly beneath body node 1.
        sequence_rows = []
        qc = (f'$modelname "{(DEST / (rig_name + ".mdl")).as_posix()}"\n'
              f'$cd "{out.as_posix()}"\n$cdtexture "{out.as_posix()}"\n'
              '$origin 0 0 0 -90\n'
              f'$eyeposition 0 0 {230 if rig_name == "kuarl" else 28}\n'
              f'$bbox {-60 if rig_name == "kuarl" else -14} {-60 if rig_name == "kuarl" else -14} 0 '
              f'{60 if rig_name == "kuarl" else 14} {60 if rig_name == "kuarl" else 14} {cfg["target_height"]:.2f}\n'
              f'$body "{rig_name}" "{rig_name}_mesh"\n')
        # Bone-local bounds follow the animated body instead of placing the
        # complete hitbox above its translated root (and missing the legs).
        for bone in sorted({bone for bone, _ in ids_for_nodes}):
            local_points = np.concatenate([np.asarray(models[mid]["vertices"], dtype=float)[:, :3] @ C.T * scale
                                           for b, mid in ids_for_nodes if b == bone])
            lo, hi = local_points.min(axis=0), local_points.max(axis=0)
            qc += f'$hbox 0 "{all_names[bone]}" ' + ' '.join(f'{v:.6f}' for v in (*lo, *hi)) + '\n'
        for mid, clip in clips.items():
            name = MOTION_NAMES.get(mid, f"anim_{mid:03x}")
            frames = clip["frames"]
            wing_clip = companion_clips.get(mid)
            wing_frames = wing_clip["frames"] if wing_clip else []
            smd = [nodes_text, "skeleton\n"]
            converted = []
            for frame_i, frame in enumerate(frames):
                pos = np.tile(np.asarray([n["offset"] for n in nodes], dtype=float), (16, 1))
                pos[0] = np.asarray(frame["position"], dtype=float)
                pos = pos @ C.T * scale
                # Kuarl's motions already include the pelvis-to-ground height.
                # Adding the reference-pose floor again lifts the entire actor.
                if rig_name != "kuarl":
                    pos[0, 2] += floor
                angles = np.asarray(frame["rotations"], dtype=float).reshape(16, 3) * (2 * math.pi / 65536.0)
                rotations = Rotation.from_euler("xyz", angles).as_matrix()
                converted.append((pos, rotations))
            if len(converted) > 1:
                # In-game navigation owns horizontal translation; retain authored bob/falls.
                root_start = converted[0][0][0].copy()
                for pos, _ in converted:
                    pos[0, :2] = root_start[:2]
            for frame_i, (pos, rotations) in enumerate(converted):
                smd.append(f"time {frame_i}\n")
                for i in range(16):
                    angles = Rotation.from_matrix(C @ rotations[i] @ C.T).as_euler("xyz")
                    smd.append(f"{i} " + " ".join(f"{x:.7f}" for x in (*pos[i], *angles)) + "\n")
                if wing_frames:
                    wing_frame = wing_frames[min(frame_i, len(wing_frames) - 1)]
                    wpos = np.tile(np.asarray([n["offset"] for n in companion_nodes], dtype=float), (7, 1))
                    wpos[0] = np.asarray(wing_frame["position"], dtype=float)
                    wpos = wpos @ C.T * scale
                    wangles = np.asarray(wing_frame["rotations"], dtype=float).reshape(7, 3) * (2 * math.pi / 65536.0)
                    wrot = Rotation.from_euler("xyz", wangles).as_matrix()
                    for node_i in range(7):
                        bone = wing_bone_map[node_i]
                        angles = Rotation.from_matrix(C @ wrot[node_i] @ C.T).as_euler("xyz")
                        smd.append(f"{bone} " + " ".join(f"{x:.7f}" for x in (*wpos[node_i], *angles)) + "\n")
                smd.append("")
            (out / f"{name}.smd").write_text("".join(smd) + "end\n", encoding="ascii")
            qc += f'$sequence {name} "{name}" fps 30\n'
            sequence_rows.append({"name": name, "motion_id": mid, "frames": len(frames),
                                  "ticks_60hz": clip["ticks_60hz"],
                                  "wing_motion_id": wing_clip["id"] if wing_clip else None,
                                  "wing_frames": len(wing_frames) if wing_frames else 0})

        (out / f"{rig_name}.qc").write_text(qc, encoding="utf8")
        result = subprocess.run([str(COMPILER), str(out / f"{rig_name}.qc")], capture_output=True,
                                text=True, check=False)
        (out / "compile.log").write_text(result.stdout + result.stderr, encoding="utf8")
        compiled = DEST / f"{rig_name}.mdl"
        if not compiled.is_file() or compiled.read_bytes()[:4] != b"IDST":
            raise RuntimeError(f"StudioMDL failed for {rig_name}:\n{result.stdout}\n{result.stderr}")

        # StudioMDL on this SDK omits STUDIO_NF_MASKED for brace textures.
        data = bytearray(compiled.read_bytes())
        if data[:4] != b"IDST":
            raise RuntimeError(f"Invalid GoldSrc MDL header: {compiled}")
        texture_count, texture_offset = struct.unpack_from("<ii", data, 180)
        masked_count = 0
        for i in range(texture_count):
            at = texture_offset + i * 80
            if data[at:at + 1] == b"{":
                flags = struct.unpack_from("<i", data, at + 64)[0]
                struct.pack_into("<i", data, at + 64, flags | 64)
                masked_count += 1
        compiled.write_bytes(data)
        bones, _ = struct.unpack_from("<ii", data, 140)
        # Header offsets: sequence count/offset = 164/168; assert compiled layout.
        seq_count, seq_offset = struct.unpack_from("<ii", data, 164)
        bone_count, bone_offset = struct.unpack_from("<ii", data, 140)
        compiled_names = [data[bone_offset + i * 112:bone_offset + i * 112 + 32].split(b"\0")[0].decode()
                          for i in range(bone_count)]
        required_bones = {all_names[0], *(all_names[b] for b, _ in ids_for_nodes)}
        if companion_nodes:
            required_bones.add("WingRoot")
        if bone_count != len(required_bones) or seq_count != len(sequence_rows) or not required_bones.issubset(compiled_names):
            raise RuntimeError(f"Compiled {rig_name} header mismatch: bones={bone_count}/{len(required_bones)}, "
                               f"sequences={seq_count}/{len(sequence_rows)}, names={compiled_names}")
        DEST.mkdir(parents=True, exist_ok=True)
        dest = DEST / compiled.name
        report = {"name": rig_name, "kind": cfg["kind"], "scale": scale, "source_height": height,
                  "target_height": cfg["target_height"], "floor_offset": floor,
                  "nodes": all_names, "compiled_bones": compiled_names, "parents": joined_parents,
                  "active_models": [mid for _, mid in ids_for_nodes],
                  "attachment_models": [mid for _, mid in attachments] +
                                       ([mid for _, mid in attachment_specs(exe, 0x46)] if companion_nodes else []),
                  "triangles": sum(len(models[mid]["faces"]) for _, mid in ids_for_nodes),
                  "sequences": sequence_rows, "compiled_bone_count": bone_count, "compiled_sequences": seq_count,
                  "compiled_textures": texture_count, "masked_textures": masked_count,
                  "installed_path": str(dest), "installed_sha256": hashlib.sha256(dest.read_bytes()).hexdigest(),
                  "source_texture_alpha": {str(k): {"transparent": v["alpha_zero"],
                                                    "partial": v["alpha_partial"]} for k, v in textures.items()}}
        (BASE / f"{rig_name}_conversion_report.json").write_text(json.dumps(report, indent=2) + "\n")
        return report
    finally:
        OUT = old_out


def main() -> None:
    extract_script = MOD / "tools" / "extract_hotd2_judgment.py"
    subprocess.run([sys.executable, str(extract_script)], check=True)
    exe = Exe(r"D:\games\House_of_the_dead_2_ISO\HOD2.EXE")
    reports = [build_one(exe, name) for name in RIGS]
    (BASE / "conversion_report.json").write_text(json.dumps(reports, indent=2) + "\n")
    for report in reports:
        print(f"{report['name']}: {report['compiled_bone_count']} bones, {report['compiled_sequences']} original clips, "
              f"{report['triangles']} triangles, {report['masked_textures']} masked textures -> {report['installed_path']}")


if __name__ == "__main__":
    main()
