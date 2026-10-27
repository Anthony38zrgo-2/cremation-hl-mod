"""Render a contact sheet of the recovered source Johnny motion clips."""
from pathlib import Path
import json

import bpy

OUT = Path(__file__).resolve().parents[1] / "assets-source" / "hotd2-johnny"
bpy.ops.wm.open_mainfile(filepath=str(OUT / "Johnny_HOTD2.blend"))
scene = bpy.context.scene
clip_positions = json.loads((OUT / "blend_validation.json").read_text(encoding="utf-8"))["clips"]
cols, tile_w, tile_h, margin = 7, 260, 360, 8
temporary_dir = OUT / ".johnny_motion_thumbnails"
if temporary_dir.exists():
    raise RuntimeError(f"Temporary output already exists: {temporary_dir}")
temporary_dir.mkdir()
scene.render.resolution_x = tile_w - 2 * margin
scene.render.resolution_y = tile_h - 38
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
manifest = []

for i, clip in enumerate(clip_positions):
    scene.frame_set((clip["start"] + clip["end"]) // 2)
    bpy.context.view_layer.update()
    thumbnail = temporary_dir / f"pose_{i:02d}.png"
    scene.render.filepath = str(thumbnail)
    bpy.ops.render.render(write_still=True)
    manifest.append({"name": clip["name"], "bank": clip["bank"], "id": clip["id"], "frame": (clip["start"] + clip["end"]) // 2, "image": thumbnail.name})
(temporary_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
print(f"Rendered {len(manifest)} source motion thumbnails to {temporary_dir}")
