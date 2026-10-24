"""Prepare the Flak Cannon viewmodel mesh from the supplied GLB.

Run with Blender's bundled Python. It keeps the donor shotgun's hand triangles,
armature and animation SMDs, then replaces only the weapon triangles.
"""
from pathlib import Path
import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / "Cremation"
SOURCE = MOD / "assets-source" / "flak_cannon_model"
GLB = MOD / "assets" / "flak_cannon.glb"
REFERENCE = SOURCE / "flak_cannon_reference_original.smd"
OUTPUT_SMD = SOURCE / "v_shotgun_reference.smd"
OUTPUT_TEXTURE = SOURCE / "flak_cannon_diffuse.png"

HAND_MATERIALS = {
    "HAND_ForeArm1.bmp",
    "rubbergloveCHROME.bmp",
    "GLOVED_knuckle.bmp",
    "GLOVE_handpak.bmp.bmp",
}
WEAPON_BONE = 30  # Bone01, the donor weapon bone parented to Bip01 R Hand.
DECIMATE_RATIO = 0.035
MODEL_SCALE = 1.1


def read_reference(path):
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    marker = lines.index("triangles")
    prefix = lines[: marker + 1]
    blocks = []
    weapon_positions = []
    i = marker + 1
    while i < len(lines) and lines[i].strip() != "end":
        material = lines[i].strip()
        block = lines[i : i + 4]
        if len(block) != 4:
            raise ValueError("Incomplete triangle at end of donor SMD")
        if material in HAND_MATERIALS:
            blocks.append(block)
        else:
            for vertex in block[1:]:
                parts = vertex.split()
                weapon_positions.append(tuple(float(value) for value in parts[1:4]))
        i += 4
    if not weapon_positions:
        raise ValueError("Could not find donor weapon geometry")
    minimum = [min(point[axis] for point in weapon_positions) for axis in range(3)]
    maximum = [max(point[axis] for point in weapon_positions) for axis in range(3)]
    target = Vector([(minimum[i] + maximum[i]) * 0.5 for i in range(3)])
    return prefix, blocks, target


def get_diffuse_image():
    material = next((mat for mat in bpy.data.materials if mat.use_nodes), None)
    if material is None:
        raise ValueError("The GLB has no node-based material")
    tree = material.node_tree
    for link in tree.links:
        if link.to_node.type == "BSDF_PRINCIPLED" and link.to_socket.name == "Base Color":
            image_node = link.from_node
            if image_node.type == "TEX_IMAGE" and image_node.image:
                return image_node.image
    raise ValueError("Could not locate the GLB base-color image")


def create_flak_triangles(obj, target):
    mesh = obj.data
    mesh.calc_loop_triangles()
    uv_layer = mesh.uv_layers.active
    if uv_layer is None:
        raise ValueError("The GLB mesh has no UV map")
    world_points = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    minimum = [min(point[i] for point in world_points) for i in range(3)]
    maximum = [max(point[i] for point in world_points) for i in range(3)]
    source_center = Vector([(minimum[i] + maximum[i]) * 0.5 for i in range(3)])

    # The GLB's long axis is +X. Rotate it so +X points toward the donor muzzle,
    # which points along -Y. Uniform scale preserves the GLB's proportions.
    scale = MODEL_SCALE
    rows = []
    for triangle in mesh.loop_triangles:
        rows.append("flak_cannon.bmp")
        for vertex_index, loop_index in zip(triangle.vertices, triangle.loops):
            position = obj.matrix_world @ mesh.vertices[vertex_index].co
            centered = position - source_center
            transformed = Vector(
                (target.x + centered.y * scale,
                 target.y - centered.x * scale,
                 target.z + centered.z * scale)
            )
            normal = mesh.loops[loop_index].normal
            transformed_normal = Vector((normal.y, -normal.x, normal.z)).normalized()
            uv = uv_layer.data[loop_index].uv
            rows.append(
                f"{WEAPON_BONE} {transformed.x:.6f} {transformed.y:.6f} {transformed.z:.6f} "
                f"{transformed_normal.x:.6f} {transformed_normal.y:.6f} {transformed_normal.z:.6f} "
                f"{uv.x:.6f} {1.0 - uv.y:.6f}"
            )
    return rows, len(mesh.loop_triangles), [maximum[i] - minimum[i] for i in range(3)]


def main():
    SOURCE.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(GLB))
    obj = next((item for item in bpy.context.scene.objects if item.type == "MESH"), None)
    if obj is None:
        raise ValueError("The GLB contains no mesh object")

    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    decimate = obj.modifiers.new("GoldSrc triangle budget", "DECIMATE")
    decimate.ratio = DECIMATE_RATIO
    decimate.use_collapse_triangulate = True
    bpy.ops.object.modifier_apply(modifier=decimate.name)

    prefix, hand_blocks, target = read_reference(REFERENCE)
    flak_rows, triangle_count, dimensions = create_flak_triangles(obj, target)
    with OUTPUT_SMD.open("w", encoding="ascii", newline="\n") as output:
        output.write("\n".join(prefix).rstrip() + "\n")
        for block in hand_blocks:
            output.write("\n".join(block) + "\n")
        output.write("\n".join(flak_rows) + "\nend\n")

    diffuse = get_diffuse_image()
    diffuse.filepath_raw = str(OUTPUT_TEXTURE)
    diffuse.file_format = "PNG"
    diffuse.save()
    print(
        "FLAK_MODEL="
        f"triangles:{triangle_count}; source_dimensions:{dimensions}; "
        f"donor_weapon_center:{tuple(round(v, 3) for v in target)}; "
        f"scale:{MODEL_SCALE}; output:{OUTPUT_SMD}"
    )


if __name__ == "__main__":
    main()
