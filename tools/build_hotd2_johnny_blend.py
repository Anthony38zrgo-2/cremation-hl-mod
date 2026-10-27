"""Build an inspectable Johnny Blender asset with the recovered HOTD2 clips."""
from __future__ import annotations

import json
import math
import struct
from pathlib import Path

import bpy
from mathutils import Euler, Vector


OUT = Path(__file__).resolve().parents[1] / "assets-source" / "hotd2-johnny"
DATA = json.loads((OUT / "Johnny_extracted.json").read_text(encoding="utf-8"))

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.name = "Johnny — HOTD2 original model and motion"
scene.render.fps = 30
main_collection = bpy.data.collections.new("JOHNNY — original assembled character")
scene.collection.children.link(main_collection)
image_cache = {}
opaque_image_cache = {}
material_cache = {}


def image_for(texture_index: int, opaque: bool):
    source_image = image_cache[texture_index]
    if not opaque:
        return source_image
    if texture_index not in opaque_image_cache:
        copy = source_image.copy()
        copy.name = source_image.name + "_opaque_source_pass"
        copy.alpha_mode = "NONE"
        copy.pack()
        opaque_image_cache[texture_index] = copy
    return opaque_image_cache[texture_index]


def make_material(words: list[int]):
    key = tuple(words[:-1])
    if key in material_cache:
        return material_cache[key]
    texture_index = words[8]
    material = bpy.data.materials.new(f"Johnny_material_{len(material_cache):03d}_tex_{texture_index:02d}")
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    shader = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    shader.inputs["Roughness"].default_value = 1.0
    if "Specular IOR Level" in shader.inputs:
        shader.inputs["Specular IOR Level"].default_value = 0.0

    tsp = words[2]
    source_alpha_pass = (tsp & 0x180000) != 0x80000
    material["source_tsp_word"] = f"0x{tsp:08X}"
    material["source_uses_texture_alpha"] = source_alpha_pass
    material["alpha_policy"] = "source TSP pass" if source_alpha_pass else "opaque pass; ignore source alpha"

    if texture_index < len(DATA["textures"]):
        texture = DATA["textures"][texture_index]
        pixfmt = texture["fmt"] & 7
        tx = nodes.new("ShaderNodeTexImage")
        tx.image = image_for(texture_index, opaque=not source_alpha_pass)
        tx.interpolation = "Closest"
        tx.extension = "REPEAT"
        diffuse = [struct.unpack("<f", struct.pack("<I", v))[0] for v in words[12:15]]
        tint = nodes.new("ShaderNodeMixRGB")
        tint.name = "Original PVR diffuse modulation"
        tint.blend_type = "MULTIPLY"
        tint.inputs[0].default_value = 1.0
        tint.inputs[2].default_value = (*diffuse, 1.0)
        links.new(tx.outputs["Color"], tint.inputs[1])
        links.new(tint.outputs["Color"], shader.inputs["Base Color"])
        if source_alpha_pass and pixfmt != 1:
            links.new(tx.outputs["Alpha"], shader.inputs["Alpha"])
            material["source_pixel_format"] = texture["format_name"]
            material["source_alpha_applied"] = True
            if hasattr(material, "surface_render_method"):
                try:
                    material.surface_render_method = "DITHERED"
                except ValueError:
                    pass
        else:
            material["source_pixel_format"] = texture["format_name"]
            material["source_alpha_applied"] = False
        if "Emission Color" in shader.inputs:
            links.new(tint.outputs["Color"], shader.inputs["Emission Color"])
            shader.inputs["Emission Strength"].default_value = 0.18
    else:
        diffuse = [struct.unpack("<f", struct.pack("<I", v))[0] for v in words[12:15]]
        shader.inputs["Base Color"].default_value = (*diffuse, 1.0)
        material["source_alpha_applied"] = False
        material["source_pixel_format"] = "no texture"

    material["original_material_words"] = json.dumps(words)
    material["texture_descriptor_index"] = texture_index
    material_cache[key] = material
    return material


for texture in DATA["textures"]:
    image = bpy.data.images.load(str(OUT / texture["path"]), check_existing=True)
    image.name = f"Johnny_tex_{texture['index']:02d}_id{texture['id']}"
    image.alpha_mode = "STRAIGHT"
    image.pack()
    image_cache[texture["index"]] = image


def create_mesh(model: dict, name: str, collection):
    mesh = bpy.data.meshes.new(name + "_mesh")
    mesh.from_pydata([v[:3] for v in model["vertices"]], [], model["faces"])
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    for material in model["materials"]:
        mesh.materials.append(make_material(material["words"]))
    uv = mesh.uv_layers.new(name="Original HOTD2 UV")
    for polygon, material_index in zip(mesh.polygons, model["face_materials"]):
        polygon.material_index = material_index
        polygon.use_smooth = True
        for loop_index in polygon.loop_indices:
            uv.data[loop_index].uv = model["vertices"][mesh.loops[loop_index].vertex_index][6:8]
    if model["vertices"]:
        mesh.normals_split_custom_set_from_vertices([v[3:6] for v in model["vertices"]])
    obj["source_model_id"] = model["id"]
    obj["source_part_index"] = model["index"]
    obj["source_model_offset"] = hex(model["source_offset"])
    return obj


bone_names = {
    0: "Root", 1: "Torso", 2: "Head", 3: "UpperArm_R", 4: "Forearm_R", 5: "Hand_R",
    6: "UpperArm_L", 7: "Forearm_L", 8: "Hand_L", 9: "Pelvis", 10: "Thigh_R",
    11: "Shin_R", 12: "Foot_R", 13: "Thigh_L", 14: "Shin_L", 15: "Foot_L",
}
rests = {}
for node in DATA["nodes"]:
    rests[node["index"]] = Vector(node["offset"]) + (rests[node["parent"]] if node["parent"] is not None else Vector((0, 0, 0)))

armature_data = bpy.data.armatures.new("Johnny — source skeleton kind 0x13 (16 nodes)")
rig = bpy.data.objects.new("Johnny_Rig_original", armature_data)
main_collection.objects.link(rig)
bpy.context.view_layer.objects.active = rig
rig.select_set(True)
bpy.ops.object.mode_set(mode="EDIT")
for node in DATA["nodes"]:
    index = node["index"]
    bone = armature_data.edit_bones.new(bone_names[index])
    bone.head = rests[index]
    bone.tail = rests[index] + Vector((0, 0.7, 0))
    bone.use_connect = False
    if node["parent"] is not None:
        bone.parent = armature_data.edit_bones[bone_names[node["parent"]]]
bpy.ops.object.mode_set(mode="OBJECT")
rig.rotation_euler = (math.pi / 2, 0, 0)
rig.show_in_front = True
armature_data.display_type = "STICK"
rig["source_skeleton_type"] = "0x13"
rig["source_node_count"] = 16
rig["coordinate_conversion"] = "Original (x,y,z) -> Blender (x,-z,y), original game units"

model_by_id = {model["id"]: model for model in DATA["models"]}
node_by_model = {node["model_id"]: node["index"] for node in DATA["nodes"] if node["model_id"]}
main_objects = []
for model_id, node_index in node_by_model.items():
    obj = create_mesh(model_by_id[model_id], f"Johnny_{bone_names[node_index]}_model_{model_id}", main_collection)
    obj.parent = rig
    obj.location = rests[node_index]
    group = obj.vertex_groups.new(name=bone_names[node_index])
    group.add(list(range(len(obj.data.vertices))), 1.0, "REPLACE")
    modifier = obj.modifiers.new("Original rigid part attachment", "ARMATURE")
    modifier.object = rig
    main_objects.append(obj)

for attachment in DATA["attachments"]:
    model = json.loads(json.dumps(model_by_id[attachment["model_id"]]))
    weights = []
    assignment = attachment["assignments"]
    for vertex_index, vertex in enumerate(model["vertices"]):
        source = assignment.get(str(vertex_index))
        bone_index = source["bone"] if source else attachment["base_bone"]
        position = source["position"] if source else vertex[:3]
        normal = source["normal"] if source else vertex[3:6]
        model["vertices"][vertex_index] = [*(Vector(position) + rests[bone_index]), *normal, *vertex[6:8]]
        weights.append(bone_index)
    obj = create_mesh(model, f"Johnny_FlexibleTorso_model_{attachment['model_id']}", main_collection)
    obj.parent = rig
    for bone_index in sorted(set(weights)):
        group = obj.vertex_groups.new(name=bone_names[bone_index])
        group.add([i for i, assigned in enumerate(weights) if assigned == bone_index], 1.0, "REPLACE")
    modifier = obj.modifiers.new("Executable deformation table assignments", "ARMATURE")
    modifier.object = rig
    obj["source_skin_definition"] = attachment["definition"]
    obj["source_deformation_vertices"] = attachment["deformation_vertices"]
    main_objects.append(obj)

# Retain every variant in the source geometry file without cluttering the assembled view.
library = bpy.data.collections.new("Biblioteca — 44 modelos originales y variantes")
scene.collection.children.link(library)
library.hide_render = True
library.hide_viewport = True
for model in DATA["models"]:
    if model["id"] not in node_by_model and model["id"] not in {a["model_id"] for a in DATA["attachments"]}:
        obj = create_mesh(model, f"Source_model_{model['id']}", library)
        obj["analysis"] = "Original alternate or damaged model from znonoo.bin; not simultaneous Johnny body geometry."

# One inspectable master action keeps every original clip in its own timeline interval.
action = bpy.data.actions.new("Johnny — original motions | timeline markers identify source clips")
rig.animation_data_create()
rig.animation_data.action = action
cursor = 1
clip_positions = []
for clip in DATA["clips"]:
    start = cursor
    scene.timeline_markers.new(f"{clip['name']} | {clip['bank']} 0x{clip['id']:03X}", frame=start)
    prior = {}
    for frame_index, values in enumerate(clip["frames"]):
        frame = start + frame_index
        for node in DATA["nodes"]:
            index = node["index"]
            pose_bone = rig.pose.bones[bone_names[index]]
            pose_bone.rotation_mode = "QUATERNION"
            angles = [value * 2 * math.pi / 65536 for value in values["rotations"][index]]
            quaternion = Euler(angles, "XYZ").to_quaternion()
            if index in prior and prior[index].dot(quaternion) < 0:
                quaternion.negate()
            prior[index] = quaternion.copy()
            pose_bone.rotation_quaternion = quaternion
            pose_bone.location = values["position"] if index == 0 else (0, 0, 0)
            pose_bone.keyframe_insert("rotation_quaternion", frame=frame, group=bone_names[index])
            pose_bone.keyframe_insert("location", frame=frame, group=bone_names[index])
    end = start + len(clip["frames"]) - 1
    scene.timeline_markers.new(f"END {clip['name']} | 0x{clip['id']:03X}", frame=end)
    clip_positions.append({"name": clip["name"], "bank": clip["bank"], "id": clip["id"], "start": start, "end": end, "frames": len(clip["frames"])})
    cursor = end + 5
for curve in getattr(action, "fcurves", []):
    for key in curve.keyframe_points:
        key.interpolation = "LINEAR"
scene.frame_start = 1
scene.frame_end = cursor - 1
scene.frame_set(1)

readme = bpy.data.texts.new("READ ME — Johnny source recovery")
readme.write(
    "Johnny / HOTD2 PC — recovered from the local game resources.\n\n"
    "Main collection: the 15 rigid models selected by original skeleton kind 0x13 plus model 7852, "
    "the flexible torso/pelvis attachment reconstructed from the executable deformation table.\n"
    "The separate hidden library retains all 44 models and damage variants in znonoo.bin.\n"
    "All 28 texture images are packed. Original UVs, per-material diffuse words, and source material alpha pass are preserved.\n"
    "Materials use source TSP flags: opaque passes ignore image alpha; alpha passes retain source ARGB1555/ARGB4444 channels.\n"
    "Action timeline contains five shared zom.bin clips and all 23 Johnny ono.bin clips. Scrub the named timeline intervals.\n"
    "See Johnny_alpha_audit.json and Johnny_alpha_audit.png for per-texture alpha and per-material pass evidence.\n"
)

# Inspection studio and framing use evaluated geometry at the first waiting frame.
bpy.context.view_layer.update()
depsgraph = bpy.context.evaluated_depsgraph_get()
points = [obj.matrix_world @ Vector(corner) for obj in main_objects for corner in obj.evaluated_get(depsgraph).bound_box]
low = Vector(tuple(min(point[axis] for point in points) for axis in range(3)))
high = Vector(tuple(max(point[axis] for point in points) for axis in range(3)))
center = (low + high) / 2
height = high.z - low.z
studio = bpy.data.collections.new("Inspection studio")
scene.collection.children.link(studio)
camera_data = bpy.data.cameras.new("Johnny_camera")
camera = bpy.data.objects.new("Johnny_camera", camera_data)
studio.objects.link(camera)
camera.location = center + Vector((height * 0.10, height * 2.8, height * 0.04))
camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()
camera_data.type = "ORTHO"
camera_data.ortho_scale = height * 1.14
scene.camera = camera
for name, offset, power, size in [
    ("Key", (-height, height, height), 2200, 8),
    ("Fill", (height, height * 0.5, height * 0.4), 1600, 8),
    ("Rim", (0, -height, height * 0.7), 1800, 6),
]:
    light_data = bpy.data.lights.new(name, "AREA")
    light_data.energy = power
    light_data.shape = "DISK"
    light_data.size = size
    light = bpy.data.objects.new(name, light_data)
    studio.objects.link(light)
    light.location = center + Vector(offset)
    light.rotation_euler = (center - light.location).to_track_quat("-Z", "Y").to_euler()
world = bpy.data.worlds.new("Neutral inspection world")
world.use_nodes = True
background = next(node for node in world.node_tree.nodes if node.type == "BACKGROUND")
background.inputs[0].default_value = (0.12, 0.12, 0.12, 1)
background.inputs[1].default_value = 0.6
scene.world = world
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = 720
scene.render.resolution_y = 960
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.filepath = str(OUT / "Johnny_preview.png")
scene.view_settings.view_transform = "Standard"
for obj in bpy.context.selected_objects:
    obj.select_set(False)
for obj in main_objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active = main_objects[0]
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type == "VIEW_3D":
            space = area.spaces.active
            space.shading.type = "MATERIAL"
            space.region_3d.view_distance = height * 1.55
            space.region_3d.view_location = center
            space.region_3d.view_rotation = camera.rotation_euler.to_quaternion()
            space.clip_end = 10000

blend_path = OUT / "Johnny_HOTD2.blend"
bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
bpy.ops.render.render(write_still=True)
report = {
    "skeleton_nodes": len(armature_data.bones), "assembled_parts": len(main_objects),
    "active_model_ids": sorted(node_by_model),
    "attachment_model_ids": [a["model_id"] for a in DATA["attachments"]],
    "all_model_variants": len(DATA["models"]), "textures": len(image_cache),
    "packed_textures": sum(bool(image.packed_file) for image in image_cache.values()),
    "materials": len(material_cache), "clips": clip_positions,
    "scene_frames": [scene.frame_start, scene.frame_end],
    "bounds": [list(low), list(high)], "alpha_audit": "Johnny_alpha_audit.json",
    "blender_version": bpy.app.version_string,
}
(OUT / "blend_validation.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
print("JOHNNY_BLEND_READY", json.dumps({k: report[k] for k in ("skeleton_nodes", "assembled_parts", "textures", "packed_textures", "materials", "scene_frames", "blender_version")}, indent=2))
