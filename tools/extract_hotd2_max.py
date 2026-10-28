"""Recover Max's HOTD2 PC geometry, original alpha textures and motion banks."""
from __future__ import annotations

import collections
import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

MOD = Path(__file__).resolve().parents[1]
BASE = MOD / "assets-source" / "hotd2-max"
SOURCE = BASE / "source"
sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, r"D:\games\House_of_the_dead_2_ISO\David_analysis\hotd2-re-reference\tools")
import extract_hotd2_johnny as shared  # noqa: E402
from tex_export import Exe, descriptors, texture_files  # noqa: E402

shared.KIND = 0x02
shared.POL_RESOURCE = "znchain.bin"
shared.TEXTURE_RESOURCE = "znchain.bin"

MODEL_FILE = "F12976_znchain.bin"
TEXTURE_FILE = "F13777_znchain.bin"
BASE_MOTION_FILE = "F12583_zom.bin"
MAX_MOTION_FILE = "F12623_cha.bin"
SKELETON_NODE_COUNT = 16
MAX_SAW_MODELS = {5: 0x1BD3, 8: 0x1BCD}
BASE_MOTIONS = {
    0x3CF: "idle", 0x3E8: "idle_alt", 0x3FE: "walk", 0x3C8: "run",
    0x3DB: "die_simple", 0x3DA: "die_back", 0x3CC: "die_forward",
    0x3DC: "die_violent", 0x3DE: "die_head", 0x3F0: "pain",
}
MAX_MOTIONS = {mid: f"chainsaw_{mid:02X}" for mid in range(0xC1, 0xCA)}
FORMAT_NAMES = {0: "ARGB1555", 1: "RGB565 (opaque)", 2: "ARGB4444"}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    for required in (MODEL_FILE, TEXTURE_FILE, BASE_MOTION_FILE, MAX_MOTION_FILE):
        if not (SOURCE / required).is_file():
            raise FileNotFoundError(SOURCE / required)
    textures_dir = BASE / "textures"
    textures_dir.mkdir(parents=True, exist_ok=True)
    exe_path = Path(r"D:\games\House_of_the_dead_2_ISO\HOD2.EXE")
    exe = Exe(str(exe_path))
    root, declared_nodes = shared.skeleton(exe, shared.KIND)
    nodes = []

    def visit(node, parent=None):
        nodes.append({"index": node.index, "model_id": node.model_id, "offset": node.offset,
                      "flags": node.flags, "parent": parent})
        for child in node.children:
            visit(child, node.index)

    visit(root)
    if declared_nodes != SKELETON_NODE_COUNT or len(nodes) != declared_nodes:
        raise RuntimeError(f"Max rig mismatch: declared={declared_nodes}, tree={len(nodes)}")

    geometry, geometry_decoding = shared.raw_or_prs(SOURCE / MODEL_FILE)
    texture_bank, texture_decoding = shared.raw_or_prs(SOURCE / TEXTURE_FILE)
    pol_index, model_ids, models = shared.model_data(exe, geometry, nodes)
    attachments = shared.attachment_data(exe, geometry, models)
    model_by_id = {model["id"]: model for model in models}
    node_by_model = {node["model_id"]: node["index"] for node in nodes if node["model_id"]}
    active_ids = set(node_by_model) | {entry["model_id"] for entry in attachments} | set(MAX_SAW_MODELS.values())
    missing = active_ids - model_by_id.keys()
    if missing:
        raise RuntimeError(f"Max rig references models outside znchain bank: {sorted(missing)}")

    texture_row = next(row for row in texture_files(exe) if row[1] == "znchain.bin")
    texture_index, _, texture_descriptor_address, texture_count = texture_row
    material_uses: dict[int, list[dict]] = collections.defaultdict(list)
    for model_id in active_ids:
        for material_index, material in enumerate(model_by_id[model_id]["materials"]):
            texture_id = material["texture"]
            if texture_id == 0xFFFFFFFF:
                continue
            if not 0 <= texture_id < texture_count:
                raise RuntimeError(f"Max model {model_id} uses invalid texture {texture_id}")
            material_uses[texture_id].append({
                "model_id": model_id,
                "node": node_by_model.get(model_id),
                "material_index": material_index,
                "tsp": f"0x{material['words'][2]:08X}",
                "uses_texture_alpha": (material["words"][2] & 0x180000) != 0x80000,
            })

    texture_records, alpha_rows = [], []
    for desc in descriptors(exe, texture_descriptor_address, texture_count):
        width, height, texels = shared.decode_texture(texture_bank, desc)
        rgba = shared.rgba_bytes(texels, desc["fmt"] & 7)
        name = f"Max_tex_{desc['index']:02d}_id{desc['id']}.png"
        Image.frombytes("RGBA", (width, height), rgba).transpose(Image.Transpose.FLIP_TOP_BOTTOM).save(textures_dir / name)
        alpha = np.frombuffer(rgba, dtype=np.uint8).reshape(-1, 4)[:, 3]
        histogram = collections.Counter(alpha.tolist())
        pixfmt = desc["fmt"] & 7
        record = {
            **desc, "format_name": FORMAT_NAMES[pixfmt], "path": f"textures/{name}",
            "rgba_sha256_unflipped": sha256(rgba), "alpha_unique": len(histogram),
            "alpha_zero": histogram.get(0, 0),
            "alpha_partial": sum(count for value, count in histogram.items() if 0 < value < 255),
            "alpha_opaque": histogram.get(255, 0), "alpha_values": sorted(histogram),
            "used_by_max": desc["index"] in material_uses,
            "material_uses": material_uses.get(desc["index"], []),
        }
        texture_records.append(record)
        alpha_rows.append({key: record[key] for key in (
            "index", "id", "w", "h", "fmt", "format_name", "used_by_max", "alpha_unique",
            "alpha_zero", "alpha_partial", "alpha_opaque", "alpha_values", "material_uses")})

    base_clips = shared.extract_motions(exe, declared_nodes, tuple(BASE_MOTIONS),
                                         SOURCE / BASE_MOTION_FILE, "zom.bin", BASE_MOTIONS)
    max_clips = shared.extract_motions(exe, declared_nodes, tuple(MAX_MOTIONS),
                                        SOURCE / MAX_MOTION_FILE, "cha.bin", MAX_MOTIONS)
    result = {
        "source": "HOTD2 PC local DATA.CAB / HOD2.EXE",
        "identity": {"name": "Max", "skeleton_kind": shared.KIND, "nodes": declared_nodes},
        "geometry": {"resource": "znchain.bin", "file_index": pol_index, "source_file": MODEL_FILE,
                     "source_sha256": sha256((SOURCE / MODEL_FILE).read_bytes()),
                     "decoded_sha256": sha256(geometry), "decoding": geometry_decoding,
                     "models_in_resource": len(model_ids), "active_node_models": len(node_by_model),
                     "active_saw_models": MAX_SAW_MODELS,
                     "active_triangles": sum(len(model_by_id[mid]["faces"]) for mid in node_by_model),
                     "attachment_models": len(attachments),
                     "attachment_triangles": sum(len(model_by_id[a["model_id"]]["faces"]) for a in attachments)},
        "texture_resource": {"resource": "znchain.bin", "file_index": texture_index,
                             "source_file": TEXTURE_FILE, "source_sha256": sha256((SOURCE / TEXTURE_FILE).read_bytes()),
                             "decoded_sha256": sha256(texture_bank), "decoding": texture_decoding,
                             "descriptor_count": texture_count,
                             "active_descriptor_count": len(material_uses)},
        "motion_sources": [
            {"bank": "zom.bin", "source_file": BASE_MOTION_FILE,
             "source_sha256": sha256((SOURCE / BASE_MOTION_FILE).read_bytes()),
             "clip_count": len(base_clips)},
            {"bank": "cha.bin", "source_file": MAX_MOTION_FILE,
             "source_sha256": sha256((SOURCE / MAX_MOTION_FILE).read_bytes()),
             "clip_count": len(max_clips)},
        ],
        "alpha_policy": "Preserve decoded texture alpha; apply it only to source materials whose TSP alpha pass is not opaque.",
        "alpha_formats": FORMAT_NAMES,
        "textures": texture_records, "models": models, "nodes": nodes,
        "attachments": attachments, "clips": base_clips + max_clips,
    }
    (BASE / "Max_extracted.json").write_text(json.dumps(result, separators=(",", ":")), encoding="utf-8")
    (BASE / "Max_alpha_audit.json").write_text(json.dumps({"alpha_formats": FORMAT_NAMES,
        "opaque_pass_rule": "TSP & 0x180000 == 0x80000; ignore texture alpha for this material",
        "textures": alpha_rows}, indent=2) + "\n", encoding="utf-8")
    hashes = {path.name: sha256(path.read_bytes()) for path in sorted(SOURCE.glob("F*.bin"))}
    hashes["F13888_chain_saw_22.wav"] = sha256((SOURCE / "F13888_chain_saw_22.wav").read_bytes())
    hashes["HOD2.EXE"] = sha256(exe_path.read_bytes())
    (BASE / "source-sha256.json").write_text(json.dumps(hashes, indent=2) + "\n", encoding="utf-8")
    summary = {
        "output": str(BASE), "skeleton_nodes": declared_nodes, "models_in_bank": len(model_ids),
        "active_parts": len(node_by_model), "attachments": len(attachments),
        "textures": texture_count, "used_textures": len(material_uses),
        "alpha_textures": sum(bool(t["alpha_zero"] or t["alpha_partial"]) for t in texture_records),
        "clips": len(base_clips) + len(max_clips),
        "max_attack_clips": [f"0x{mid:02X}" for mid in MAX_MOTIONS],
        "saw_models": MAX_SAW_MODELS,
    }
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
