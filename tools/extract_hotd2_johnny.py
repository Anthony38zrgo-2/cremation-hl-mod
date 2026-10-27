"""Recover Johnny's original HOTD2 PC model, textures and animation banks."""
from __future__ import annotations

import collections
import hashlib
import json
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ISO_ANALYSIS = Path(r"D:\games\House_of_the_dead_2_ISO\David_analysis")
OUT = Path(__file__).resolve().parents[1] / "assets-source" / "hotd2-johnny"
REF_TOOLS = ISO_ANALYSIS / "hotd2-re-reference" / "tools"
sys.path.insert(0, str(REF_TOOLS))

from prs import decompress  # noqa: E402
from pol_parse import header_count, parse_model, pol_files, read_header, triangles  # noqa: E402
from skel_pose import frame_at, mot_files, skeleton  # noqa: E402
from tex_export import Exe, decode_texture, descriptors, rgba_bytes, texture_files  # noqa: E402


KIND = 0x13
POL_RESOURCE = "znonoo.bin"
TEXTURE_RESOURCE = "znonoo.bin"
BASE_MOTION_IDS = (0x3CF, 0x3E8, 0x3FE, 0x3C8, 0x3F0, 0x3DB, 0x3DA, 0x3CC, 0x3DC, 0x3DE)
JOHNNY_MOTION_IDS = tuple(range(0x306, 0x319)) + tuple(range(0x3EB, 0x3EF))
FORMAT_NAMES = {0: "ARGB1555", 1: "RGB565 (opaque)", 2: "ARGB4444"}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def raw_or_prs(path: Path) -> tuple[bytes, str]:
    raw = path.read_bytes()
    if len(raw) >= 8:
        wanted = struct.unpack_from("<I", raw)[0]
        try:
            decoded = decompress(raw[4:])
            if len(decoded) == wanted:
                return decoded, "PRS"
        except (ValueError, IndexError):
            pass
    return raw, "raw motion/model/texture stream"


def model_data(exe: Exe, data: bytes, nodes: list) -> tuple[int, list[int], list[dict]]:
    records = [(n, name, ids) for n, name, ids in pol_files(exe) if name == POL_RESOURCE and ids]
    if len(records) != 1:
        raise RuntimeError(f"Expected one tabled {POL_RESOURCE} model bank, got {records}")
    file_no, _, ids = records[0]
    headers = read_header(data, header_count(data))
    if len(headers) - 1 != len(ids):
        raise RuntimeError(f"model/header count mismatch: {len(headers)-1} != {len(ids)}")
    active_ids = {n["model_id"] for n in nodes if n["model_id"]}
    if not active_ids.issubset(set(ids)):
        raise RuntimeError(f"Johnny skeleton references missing models: {sorted(active_ids-set(ids))}")
    models = []
    for k, model_id in enumerate(ids):
        hdr, meshes = parse_model(data, headers[k][2], headers[k + 1][0])
        vertices, vertex_lookup, faces, materials, face_materials = [], {}, [], [], []
        for mesh in meshes:
            material_index = len(materials)
            words = list(mesh.material)
            materials.append({"words": words, "texture": words[8]})
            for tri in triangles(mesh):
                pos = np.asarray([v[:3] for v in tri], dtype=np.float64)
                normal = np.cross(pos[1] - pos[0], pos[2] - pos[0])
                if np.linalg.norm(normal) < 1e-10:
                    continue
                if np.dot(normal, np.asarray([v[3:6] for v in tri]).mean(0)) < 0:
                    tri = (tri[0], tri[2], tri[1])
                face = []
                for vertex in tri:
                    if not all(np.isfinite(x) for x in vertex):
                        raise RuntimeError(f"non-finite vertex in model {model_id}")
                    if vertex not in vertex_lookup:
                        vertex_lookup[vertex] = len(vertices)
                        vertices.append(vertex)
                    face.append(vertex_lookup[vertex])
                faces.append(face)
                face_materials.append(material_index)
        models.append({
            "index": k, "id": model_id, "source_offset": headers[k][2], "header": hdr,
            "vertices": vertices, "faces": faces, "materials": materials,
            "face_materials": face_materials,
        })
    return file_no, ids, models


def attachment_data(exe: Exe, geometry: bytes, models: list[dict]) -> list[dict]:
    model_by_id = {m["id"]: m for m in models}
    table = exe.u32(0x0052ED08 + KIND * 4)
    count, definitions = struct.unpack("<II", exe.bytes(table, 8))
    attachments = []
    for attachment_index in range(count):
        definition = exe.u32(definitions + 4 * attachment_index)
        words = struct.unpack("<14I", exe.bytes(definition, 56))
        model_id, vertex_table = words[:2]
        if model_id not in model_by_id:
            raise RuntimeError(f"attachment model {model_id} is not in Johnny's model bank")
        row = [exe.u32(0x004ED1E0 + attachment_index * 20 + j * 4) & 15 for j in range(5)]
        offset, size, shorts, pointers = struct.unpack("<IIhh", exe.bytes(vertex_table + 12, 12))
        stride = shorts * 2 + pointers * 4
        if stride <= 0 or size % stride:
            raise RuntimeError(f"invalid Johnny deformation table {attachment_index}")
        logical_count = size // stride
        offsets = []
        for vertex in range(logical_count):
            links = []
            for link in range(pointers):
                value = exe.u32(vertex_table + offset + vertex * stride + shorts * 2 + link * 4)
                if value == 0xFFFFFFFF:
                    break
                links.append(value)
            offsets.append(links)

        model = model_by_id[model_id]
        lookup = {tuple(v): i for i, v in enumerate(model["vertices"])}
        assignments, missing, conflicts = {}, 0, 0
        for record in range(4):
            vector_count, vector_address, group_address = words[2 + 3 * record:5 + 3 * record]
            if not vector_count:
                continue
            vectors = list(struct.iter_unpack("<6f", exe.bytes(vector_address, vector_count * 24)))
            groups = struct.unpack(f"<{logical_count}b", exe.bytes(group_address, logical_count))
            for vertex, group in enumerate(groups):
                if not 0 <= group < vector_count:
                    continue
                for offset in offsets[vertex]:
                    source_vertex = struct.unpack_from("<8f", geometry, model["source_offset"] + offset)
                    vertex_index = lookup.get(source_vertex)
                    if vertex_index is None:
                        missing += 1
                        continue
                    value = {"bone": row[record], "position": vectors[group][:3], "normal": vectors[group][3:]}
                    if vertex_index in assignments and assignments[vertex_index] != value:
                        conflicts += 1
                    assignments[vertex_index] = value
        if missing or conflicts:
            raise RuntimeError(f"attachment {model_id} mapping mismatch: missing={missing}, conflicts={conflicts}")
        attachments.append({
            "model_id": model_id, "base_bone": row[4],
            "assignments": assignments, "definition": hex(definition),
            "deformation_vertices": logical_count,
        })
    return attachments


def extract_motions(exe: Exe, skeleton_nodes: int, ids: tuple[int, ...], path: Path, bank_name: str, labels: dict) -> list[dict]:
    bank_rows = [(n, name, motion_ids) for n, name, motion_ids in mot_files(exe) if name == bank_name]
    if len(bank_rows) != 1:
        raise RuntimeError(f"Expected one {bank_name} bank")
    bank_index, _, bank_ids = bank_rows[0]
    data, _ = raw_or_prs(path)
    offsets = list(struct.unpack_from(f"<{len(bank_ids)}I", data)) + [len(data)]
    stride = (12 + 6 * skeleton_nodes + 3) & ~3
    clips = []
    for motion_id in ids:
        if motion_id not in bank_ids:
            raise RuntimeError(f"motion 0x{motion_id:03X} missing from {bank_name}")
        k = bank_ids.index(motion_id)
        block = data[offsets[k]:offsets[k + 1]]
        ticks = exe.s16(0x004E07D0 + 2 * motion_id)
        frame_count = (ticks - 1) // 2 + 2
        expected_size = 4 + frame_count * stride
        if len(block) != expected_size:
            raise RuntimeError(f"motion 0x{motion_id:03X}: bank size {len(block)} != {expected_size} for {skeleton_nodes} bones")
        frames = []
        for frame in range(frame_count):
            position, rotations = frame_at(block, skeleton_nodes, frame)
            frames.append({"position": position, "rotations": rotations})
        clips.append({
            "name": labels.get(motion_id, f"Johnny motion 0x{motion_id:03X}"),
            "id": motion_id, "bank": bank_name, "bank_index": bank_index,
            "ticks_60hz": ticks, "frames": frames,
        })
    return clips


def alpha_sheet(textures: list[dict], rgba_data: dict[int, bytes]) -> None:
    tile_w, tile_h, columns = 280, 178, 7
    rows = (len(textures) + columns - 1) // columns
    sheet = Image.new("RGB", (tile_w * columns, tile_h * rows), (30, 30, 30))
    draw = ImageDraw.Draw(sheet)
    for tex in textures:
        i = tex["index"]
        rgba = Image.frombytes("RGBA", (tex["w"], tex["h"]), rgba_data[i]).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
        rgb_preview = Image.new("RGBA", rgba.size, (48, 48, 48, 255))
        rgb_preview.alpha_composite(rgba)
        rgb_preview.thumbnail((116, 116), Image.Resampling.NEAREST)
        alpha = rgba.getchannel("A").convert("RGB")
        alpha.thumbnail((116, 116), Image.Resampling.NEAREST)
        x, y = (i % columns) * tile_w, (i // columns) * tile_h
        draw.text((x + 5, y + 4), f"{i:02d} id {tex['id']} {tex['w']}x{tex['h']} {tex['format_name']}", fill="white")
        draw.text((x + 5, y + 23), f"alpha: {tex['alpha_unique']} values; transparent={tex['alpha_zero']}; partial={tex['alpha_partial']}", fill=(210, 210, 210))
        sheet.paste(rgb_preview.convert("RGB"), (x + 5, y + 48))
        sheet.paste(alpha, (x + 128, y + 48))
        draw.text((x + 30, y + 166), "color / alpha mask", fill=(200, 200, 200))
    sheet.save(OUT / "Johnny_alpha_audit.png")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    textures_dir = OUT / "textures"
    textures_dir.mkdir(exist_ok=True)
    source_dir = OUT / "source"
    exe_path = ISO_ANALYSIS.parent / "HOD2.EXE"
    exe = Exe(str(exe_path))
    root, node_count = skeleton(exe, KIND)
    nodes = []

    def visit(node, parent=None):
        nodes.append({"index": node.index, "model_id": node.model_id, "offset": node.offset, "flags": node.flags, "parent": parent})
        for child in node.children:
            visit(child, node.index)

    visit(root)
    if node_count != len(nodes) or node_count != 16:
        raise RuntimeError(f"unexpected Johnny rig dimensions: declared={node_count}, tree={len(nodes)}")

    source_model = source_dir / "F12992_znonoo.bin"
    source_texture = source_dir / "F13793_znonoo.bin"
    source_base = source_dir / "F12583_zom.bin"
    source_special = source_dir / "F12597_ono.bin"
    geometry, model_compression = raw_or_prs(source_model)
    texture_bank, texture_compression = raw_or_prs(source_texture)
    pol_index, model_ids, models = model_data(exe, geometry, nodes)
    attachments = attachment_data(exe, geometry, models)

    texture_row = next(v for v in texture_files(exe) if v[1] == TEXTURE_RESOURCE)
    texture_index, _, texture_descriptor_address, texture_count = texture_row
    used_textures = set()
    material_uses: dict[int, list[dict]] = collections.defaultdict(list)
    model_by_id = {m["id"]: m for m in models}
    node_by_model = {n["model_id"]: n["index"] for n in nodes if n["model_id"]}
    active_ids = set(node_by_model) | {a["model_id"] for a in attachments}
    for model_id in active_ids:
        for material_index, material in enumerate(model_by_id[model_id]["materials"]):
            ti = material["texture"]
            if ti == 0xFFFFFFFF:
                continue
            if not 0 <= ti < texture_count:
                raise RuntimeError(f"model {model_id} uses invalid texture descriptor {ti}")
            used_textures.add(ti)
            material_uses[ti].append({
                "model_id": model_id, "node": node_by_model.get(model_id),
                "material_index": material_index,
                "tsp": f"0x{material['words'][2]:08X}",
                "uses_texture_alpha": (material["words"][2] & 0x180000) != 0x80000,
            })

    textures, rgba_data, alpha_rows = [], {}, []
    for desc in descriptors(exe, texture_descriptor_address, texture_count):
        width, height, texels = decode_texture(texture_bank, desc)
        rgba = rgba_bytes(texels, desc["fmt"] & 7)
        rgba_data[desc["index"]] = rgba
        image_name = f"Johnny_tex_{desc['index']:02d}_id{desc['id']}.png"
        Image.frombytes("RGBA", (width, height), rgba).transpose(Image.Transpose.FLIP_TOP_BOTTOM).save(textures_dir / image_name)
        alpha = np.frombuffer(rgba, dtype=np.uint8).reshape(-1, 4)[:, 3]
        hist = collections.Counter(alpha.tolist())
        pixfmt = desc["fmt"] & 7
        record = {
            **desc, "format_name": FORMAT_NAMES[pixfmt], "path": f"textures/{image_name}",
            "rgba_sha256_unflipped": sha256(rgba), "alpha_unique": len(hist),
            "alpha_zero": hist.get(0, 0),
            "alpha_partial": sum(v for a, v in hist.items() if 0 < a < 255),
            "alpha_opaque": hist.get(255, 0),
            "alpha_values": sorted(hist), "used_by_johnny": desc["index"] in used_textures,
            "material_uses": material_uses.get(desc["index"], []),
        }
        textures.append(record)
        alpha_rows.append({k: record[k] for k in ("index", "id", "w", "h", "fmt", "format_name", "used_by_johnny", "alpha_unique", "alpha_zero", "alpha_partial", "alpha_opaque", "alpha_values", "material_uses")})
    alpha_sheet(textures, rgba_data)

    base_labels = {0x3CF: "Waiting", 0x3FE: "Walk", 0x3C8: "Run",
                   0x3DB: "Death simple", 0x3DA: "Death backward", 0x3CC: "Death forward",
                   0x3DC: "Death violent", 0x3DE: "Death headshot"}
    axe_labels = {0x307: "Axe throw", 0x310: "Johnny run variant"}
    clips = extract_motions(exe, node_count, BASE_MOTION_IDS, source_base, "zom.bin", base_labels)
    clips += extract_motions(exe, node_count, JOHNNY_MOTION_IDS, source_special, "ono.bin", axe_labels)

    active_triangles = sum(len(model_by_id[mid]["faces"]) for mid in node_by_model)
    attachment_triangles = sum(len(model_by_id[a["model_id"]]["faces"]) for a in attachments)
    result = {
        "source": "HOTD2 PC local DATA.CAB / HOD2.EXE",
        "identity": {"name": "Johnny", "skeleton_kind": KIND, "nodes": node_count},
        "geometry": {"resource": POL_RESOURCE, "file_index": pol_index, "source_file": source_model.name,
                     "source_sha256": sha256(source_model.read_bytes()), "decoded_sha256": sha256(geometry),
                     "decoding": model_compression, "models_in_resource": len(model_ids),
                     "active_node_models": len(node_by_model), "active_triangles": active_triangles,
                     "attachment_models": len(attachments), "attachment_triangles": attachment_triangles},
        "texture_resource": {"resource": TEXTURE_RESOURCE, "file_index": texture_index,
                             "source_file": source_texture.name, "source_sha256": sha256(source_texture.read_bytes()),
                             "decoded_sha256": sha256(texture_bank), "decoding": texture_compression,
                             "descriptor_count": texture_count, "active_descriptor_count": len(used_textures)},
        "motion_sources": [
            {"bank": "zom.bin", "source_file": source_base.name, "source_sha256": sha256(source_base.read_bytes()),
             "decoded_sha256": sha256(raw_or_prs(source_base)[0]), "clip_count": len(BASE_MOTION_IDS)},
            {"bank": "ono.bin", "source_file": source_special.name, "source_sha256": sha256(source_special.read_bytes()),
             "decoded_sha256": sha256(raw_or_prs(source_special)[0]), "clip_count": len(JOHNNY_MOTION_IDS)},
        ],
        "alpha_policy": "Preserve decoded alpha for every texture. Use it only when the source TSP pass is not opaque (TSP & 0x180000 != 0x80000); opaque materials must ignore it.",
        "textures": textures, "models": models, "nodes": nodes,
        "attachments": attachments, "clips": clips,
    }
    (OUT / "Johnny_extracted.json").write_text(json.dumps(result, separators=(",", ":")), encoding="utf-8")
    (OUT / "Johnny_alpha_audit.json").write_text(json.dumps({
        "alpha_formats": FORMAT_NAMES,
        "opaque_pass_rule": "TSP & 0x180000 == 0x80000; ignore texture alpha for this material",
        "textures": alpha_rows,
    }, indent=2), encoding="utf-8")
    hashes = {f.name: sha256(f.read_bytes()) for f in sorted(source_dir.glob("F*.bin"))}
    hashes["HOD2.EXE"] = sha256(exe_path.read_bytes())
    (OUT / "source-sha256.json").write_text(json.dumps(hashes, indent=2) + "\n", encoding="utf-8")
    (OUT / "README.md").write_text(
        "# Johnny — HOTD2 PC\n\n"
        "Recovered from the local ISO using its executable resource tables. `Johnny_extracted.json` stores "
        "the original 16-node hierarchy, all 44 models in the geometry bank, the 15 rigid body parts, "
        "the executable-defined flexible attachment, original UVs/material words, 28 texture descriptors, "
        f"and {len(clips)} decoded clips from `zom.bin` and `ono.bin`, including shared standing deaths.\n\n"
        "The source geometry bank resolves to model IDs 7850–7892 plus 585; Johnny kind 0x13 selects 15 "
        "rigid parts and deformation model 7852. The assembled rigid parts contain " + str(active_triangles) +
        " triangles, plus " + str(attachment_triangles) + " in the skin attachment.\n\n"
        "Alpha audit: `Johnny_alpha_audit.json` reports every pixel format, alpha histogram, texture use, "
        "and material TSP alpha decision; `Johnny_alpha_audit.png` shows decoded color and alpha masks. "
        "ARGB1555 masks retain binary transparency, ARGB4444 retains all decoded alpha levels, and RGB565 "
        "is opaque. Materials whose source pass is opaque ignore alpha even when their image contains it.\n\n"
        "Raw local source files and SHA-256 values are retained under `source/`; the ISO binaries were not modified.\n",
        encoding="utf-8",
    )
    print(json.dumps({
        "out": str(OUT), "skeleton_nodes": node_count,
        "models_in_bank": len(models), "active_parts": len(node_by_model),
        "active_triangles": active_triangles, "attachments": len(attachments),
        "textures": texture_count, "used_textures": len(used_textures),
        "clips": len(clips), "alpha_texture_count": sum(bool(t["alpha_zero"] or t["alpha_partial"]) for t in textures),
        "formats": dict(collections.Counter(t["format_name"] for t in textures)),
    }, indent=2))


if __name__ == "__main__":
    main()
