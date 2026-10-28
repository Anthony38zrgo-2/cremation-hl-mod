"""Render quick static GoldSrc-basis inspection images from assembled SMDs."""
from pathlib import Path
import bpy
import math
import numpy as np
from mathutils import Euler, Vector

MOD = Path(__file__).resolve().parents[1]
BASE = MOD / "assets-source" / "hotd2-judgment" / "goldsrc"
PREVIEW = MOD / "assets-source" / "hotd2-judgment" / "previews"
PREVIEW.mkdir(parents=True, exist_ok=True)


def parse_pose(path: Path):
    lines = path.read_text(encoding="ascii").splitlines()
    hierarchy = {}
    start = lines.index("nodes") + 1
    while lines[start] != "end":
        row = lines[start].split(maxsplit=2)
        hierarchy[int(row[0])] = (row[1].strip('"'), int(row[2]))
        start += 1
    frames = []
    start = lines.index("skeleton") + 1
    current = None
    for line in lines[start:]:
        if line == "end":
            break
        if line.startswith("time "):
            if current is not None:
                frames.append(current)
            current = {}
            continue
        if current is not None and line:
            row = line.split()
            current[int(row[0])] = np.asarray([float(v) for v in row[1:]], dtype=float)
    if current is not None:
        frames.append(current)
    return hierarchy, frames


def global_matrices(hierarchy, pose):
    matrices = {}
    for index in hierarchy:
        values = pose[index]
        local = np.eye(4)
        local[:3, :3] = np.asarray(Euler(values[3:6], "XYZ").to_matrix())
        local[:3, 3] = values[:3]
        parent = hierarchy[index][1]
        matrices[index] = local if parent < 0 else matrices[parent] @ local
    return matrices


def load_smd_mesh(folder: Path, stem: str, sequence: str):
    path = folder / f"{stem}_mesh.smd"
    lines = path.read_text(encoding="ascii").splitlines()
    start = lines.index("triangles") + 1
    vertices, faces, uvcoords, material_names, bones = [], [], [], [], []
    index = 0
    while start < len(lines):
        material = lines[start].strip()
        if material == "end":
            break
        material_names.append(material)
        face = []
        uvs = []
        for line in lines[start + 1:start + 4]:
            values = line.split()
            pos = tuple(float(v) for v in values[1:4])
            uv = tuple(float(v) for v in values[7:9])
            vertices.append(pos)
            bones.append(int(values[0]))
            uvs.append(uv)
            face.append(index)
            index += 1
        faces.append((material, face, uvs))
        start += 4

    mesh = bpy.data.meshes.new(stem + "_inspection_mesh")
    mesh.from_pydata(vertices, [], [face for _, face, _ in faces])
    mesh.update()
    obj = bpy.data.objects.new(stem + "_assembled", mesh)
    bpy.context.scene.collection.objects.link(obj)
    unique = list(dict.fromkeys(material_names))
    by_name = {}
    for name in unique:
        mat = bpy.data.materials.new(name)
        mat.use_nodes = True
        nodes, links = mat.node_tree.nodes, mat.node_tree.links
        shader = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
        image_path = folder / name
        image = bpy.data.images.load(str(image_path), check_existing=True)
        if name.startswith("{"):
            pixels = list(image.pixels)
            for i in range(0, len(pixels), 4):
                if pixels[i] < .002 and pixels[i + 1] < .002 and pixels[i + 2] > .995:
                    pixels[i + 3] = 0.0
            image.pixels[:] = pixels
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = image
        tex.interpolation = "Closest"
        links.new(tex.outputs["Color"], shader.inputs["Base Color"])
        links.new(tex.outputs["Alpha"], shader.inputs["Alpha"])
        if hasattr(mat, "surface_render_method"):
            mat.surface_render_method = "DITHERED"
        by_name[name] = len(mesh.materials)
        mesh.materials.append(mat)
    layer = mesh.uv_layers.new(name="SMD UV")
    for polygon, (name, face, uvs) in zip(mesh.polygons, faces):
        polygon.material_index = by_name[name]
        for loop_i, uv in zip(polygon.loop_indices, uvs):
            layer.data[loop_i].uv = uv
    bind_hierarchy, bind_frames = parse_pose(path)
    anim_path = folder / f"{sequence}.smd"
    anim_hierarchy, anim_frames = parse_pose(anim_path)
    bind = global_matrices(bind_hierarchy, bind_frames[0])
    pose = global_matrices(anim_hierarchy, anim_frames[min(1, len(anim_frames) - 1)])
    for vertex, bone in zip(mesh.vertices, bones):
        point = np.asarray([*vertex.co, 1.0])
        vertex.co = (pose[bone] @ np.linalg.inv(bind[bone]) @ point)[:3]
    mesh.update()
    return obj


def render(stem: str):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 24
    scene.render.resolution_x = 900
    scene.render.resolution_y = 1050
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    scene.world = bpy.data.worlds.new(stem + "_world")
    scene.world.color = (0.3, 0.3, 0.3)
    folder = BASE / stem
    sequence = "standing" if stem == "kuarl" else "hover"
    obj = load_smd_mesh(folder, stem, sequence)
    bpy.context.view_layer.update()
    points = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    min_z = min(v.z for v in points)
    max_z = max(v.z for v in points)
    center = sum(points, Vector()) / len(points)
    size = max(max(v.x for v in points) - min(v.x for v in points),
               max(v.y for v in points) - min(v.y for v in points), max_z - min_z)
    bpy.ops.mesh.primitive_plane_add(size=size * 5)
    plane = bpy.context.object
    plane.name = "ground"
    plane.location = (center.x, center.y, min_z - 0.6)
    plane.select_set(False)
    mat = bpy.data.materials.new("charcoal backdrop")
    mat.diffuse_color = (0.025, 0.03, 0.035, 1)
    plane.data.materials.append(mat)

    def aim(obj_camera, target):
        obj_camera.rotation_euler = (Vector(target) - obj_camera.location).to_track_quat("-Z", "Y").to_euler()

    camera_data = bpy.data.cameras.new("inspection_camera")
    camera = bpy.data.objects.new("inspection_camera", camera_data)
    scene.collection.objects.link(camera)
    camera.location = (center.x + size * 2.8, center.y, center.z + size * 0.18)
    aim(camera, center)
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = max(size * 1.23, 8)
    scene.camera = camera
    for idx, (loc, energy, color, size_light) in enumerate([
        ((center.x + size, center.y - size, center.z + size * 1.4), 24000, (1.0, .82, .70), size),
        ((center.x - size, center.y - size * .2, center.z + size * .5), 16000, (.56, .72, 1.0), size * .8),
        ((center.x, center.y + size, center.z + size), 18000, (1.0, .35, .24), size * .9),
    ]):
        data = bpy.data.lights.new(f"inspection_light_{idx}", "AREA")
        data.energy, data.color, data.shape, data.size = energy, color, "DISK", size_light
        light = bpy.data.objects.new(data.name, data)
        scene.collection.objects.link(light)
        light.location = loc
        aim(light, center)
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "Medium High Contrast"
    scene.render.filepath = str(PREVIEW / f"{stem}_assembled.png")
    bpy.ops.render.render(write_still=True)


for character in ("kuarl", "zeal"):
    render(character)
