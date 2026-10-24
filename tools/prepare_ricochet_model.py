"""Derive forward-positioned Panthergun variants without changing animations."""
from pathlib import Path
import math
import struct as st

MOD = Path(__file__).resolve().parents[1]
SOURCE = MOD / 'assets-source/additional_weapons/panthergun/v_HGun_blue.mdl'
OUT = MOD / 'assets-source/additional_weapons/ricochet_variants'


def build_assets():
    blue = bytearray(SOURCE.read_bytes())
    def integer(data, offset):
        return st.unpack_from('<i', data, offset)[0]
    # Model X is forward in the studio entity coordinate system. Moving the
    # skeleton root preserves the original hands, bone weights and sequences.
    bones = integer(blue, 144)
    for n in range(integer(blue, 140)):
        bone = bones + n * 112
        if integer(blue, bone + 32) == -1:
            x = st.unpack_from('<f', blue, bone + 64)[0]
            st.pack_into('<f', blue, bone + 64, x + 6.0)
    textures, texture_index = integer(blue, 180), integer(blue, 184)
    texture_payloads = []
    texture_headers = []
    for n in range(textures):
        tex = texture_index + n * 80
        w, h, pixels = st.unpack_from('<3i', blue, tex + 68)
        payload = bytearray(blue[pixels:pixels + w*h + 768])
        for c in range(256):
            at = w*h + c*3
            r, g, b = payload[at:at+3]
            payload[at:at+3] = bytes((b,g,r))
        texture_payloads.append(payload)
        texture_headers.append(bytearray(blue[tex:tex+80]))
    # GoldSrc discards the pixel-data tail after uploading textures. All studio
    # structures must precede texturedataindex, including added meshes/skins.
    red = bytearray(blue[:integer(blue, 188)])

    def append(payload):
        while len(red) % 4:
            red.append(0)
        at = len(red)
        red.extend(payload)
        return at

    # An additional expanded surface uses additive fullbright material. The
    # original red surface keeps its opaque material and original geometry.
    body_index = integer(red, 208)
    model_index = integer(red, body_index + 72)
    model = bytearray(red[model_index:model_index + 112])
    meshes, mesh_index, count, bone_index, vertex_index, normals, normal_bones, normal_index = st.unpack_from('<8i', model, 72)
    directions = [[0.0, 0.0, 0.0] for _ in range(count)]
    shell_meshes = bytearray()
    for n in range(meshes):
        mesh = bytearray(red[mesh_index + n * 20:mesh_index + (n + 1) * 20])
        cursor = integer(mesh, 4)
        while True:
            strip = st.unpack_from('<h', red, cursor)[0]
            cursor += 2
            if strip == 0:
                break
            for _ in range(abs(strip)):
                vi, ni, _, _ = st.unpack_from('<4h', red, cursor)
                cursor += 8
                if red[bone_index + vi] == red[normal_bones + ni]:
                    normal = st.unpack_from('<3f', red, normal_index + ni * 12)
                    for axis in range(3):
                        directions[vi][axis] += normal[axis]
        # Rebase shell triangle vertex indices into the combined vertex array.
        start = integer(mesh, 4)
        commands = bytearray(red[start:cursor])
        at = 0
        while True:
            strip = st.unpack_from('<h', commands, at)[0]
            at += 2
            if not strip:
                break
            for _ in range(abs(strip)):
                vi = st.unpack_from('<h', commands, at)[0]
                st.pack_into('<h', commands, at, vi + count)
                at += 8
        st.pack_into('<i', mesh, 4, append(commands))
        st.pack_into('<i', mesh, 8, textures)
        shell_meshes.extend(mesh)
    verts = bytearray()
    for n, direction in enumerate(directions):
        v = st.unpack_from('<3f', red, vertex_index + n * 12)
        length = math.sqrt(sum(x*x for x in direction)) or 1
        verts.extend(st.pack('<3f', *(v[a] + .4 * direction[a] / length for a in range(3))))
    original_vertices = bytes(red[vertex_index:vertex_index + count * 12])
    vertex_bones = bytes(red[bone_index:bone_index + count])
    original_meshes = bytes(red[mesh_index:mesh_index + meshes * 20])
    st.pack_into('<i', model, 88, append(original_vertices + verts))
    st.pack_into('<i', model, 84, append(vertex_bones * 2))
    st.pack_into('<i', model, 80, count * 2)
    st.pack_into('<i', model, 76, append(original_meshes + shell_meshes))
    st.pack_into('<i', model, 72, meshes * 2)
    # Keep one body part, as expected by Ravenholm's custom viewmodel renderer.
    red[model_index:model_index + 112] = model
    texture_headers.append(bytearray(st.pack('<64s4i', b'red_envelope.bmp', 2 | 4 | 32, 32, 32, 0)))
    texture_payloads.append(bytes(32*32) + bytes([90,3,3])*256)
    table_index = append(b''.join(texture_headers))
    st.pack_into('<2i', red, 180, textures + 1, table_index)
    skins = append(st.pack('<' + 'h' * (textures + 1), *range(textures + 1)))
    st.pack_into('<3i', red, 192, textures + 1, 1, skins)
    while len(red) % 4:
        red.append(0)
    st.pack_into('<i', red, 188, len(red))
    for n, payload in enumerate(texture_payloads):
        pixel_index = append(payload)
        st.pack_into('<i', red, table_index + n*80 + 76, pixel_index)
    st.pack_into('<i', red, 72, len(red))
    OUT.mkdir(parents=True, exist_ok=True)
    # Reuse the same safe envelope geometry for primary mode. Restore the
    # source blue surface textures and color only its additive shell cyan.
    cyan = bytearray(red)
    for n in range(textures):
        old_tex = texture_index + n * 80
        w, h, old_pixels = st.unpack_from('<3i', blue, old_tex + 68)
        new_pixels = integer(cyan, table_index + n * 80 + 76)
        cyan[new_pixels:new_pixels + w*h + 768] = blue[old_pixels:old_pixels + w*h + 768]
    shell_tex = table_index + textures * 80
    cyan[shell_tex:shell_tex + 64] = b'cyan_envelope.bmp'.ljust(64, b'\0')
    shell_pixels = integer(cyan, shell_tex + 76)
    palette = shell_pixels + 32*32
    cyan[palette:palette + 768] = bytes([3,65,90])*256
    (OUT / 'v_ricochet_blue.mdl').write_bytes(cyan)
    (OUT / 'v_ricochet_red.mdl').write_bytes(red)


if __name__ == '__main__':
    build_assets()
