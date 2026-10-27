"""Blender background QA of the installed MDL readback, with backface culling."""
from pathlib import Path
import bpy,json
from mathutils import Vector
BASE=Path(__file__).resolve().parents[1]/'assets-source/hotd2-steve'
d=json.loads((BASE/'compiled-preview.json').read_text())
bpy.ops.wm.read_factory_settings(use_empty=True)
materials=[]
for i,path in enumerate(d['textures']):
    mat=bpy.data.materials.new(f'Compiled_texture_{i}');mat.use_nodes=True;mat.use_backface_culling=True
    shader=next(n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED');shader.inputs['Roughness'].default_value=1;shader.inputs['Specular IOR Level'].default_value=0
    tx=mat.node_tree.nodes.new('ShaderNodeTexImage');tx.image=bpy.data.images.load(path);tx.interpolation='Closest';tx.image.pack()
    mat.node_tree.links.new(tx.outputs['Color'],shader.inputs['Base Color']);mat.node_tree.links.new(tx.outputs['Alpha'],shader.inputs['Alpha'])
    mat.node_tree.links.new(tx.outputs['Color'],shader.inputs['Emission Color']);shader.inputs['Emission Strength'].default_value=.3;materials.append(mat)
for i,entry in enumerate(d['poses']):
    mesh=bpy.data.meshes.new(entry['name']);mesh.from_pydata(entry['vertices'],[],[list(reversed(f)) for f in entry['faces']]);mesh.update();obj=bpy.data.objects.new(entry['name'],mesh);bpy.context.collection.objects.link(obj);obj.location.y=(i-1)*80
    for mat in materials:mesh.materials.append(mat)
    layer=mesh.uv_layers.new(name='Compiled UV')
    for face,uv,mat in zip(mesh.polygons,entry['uv'],entry['materials']):
        face.material_index=mat;face.use_smooth=True
        for loop,coord in zip(face.loop_indices,reversed(uv)):layer.data[loop].uv=coord
    bpy.ops.object.text_add(location=(4,obj.location.y,-8));label=bpy.context.object;label.data.body=entry['name'];label.data.align_x='CENTER';label.data.size=5;label.rotation_euler=(1.5707963,0,1.5707963)
for loc,power in [((180,-120,230),1400000),((120,180,160),900000),((-100,0,150),800000)]:
    bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=160;light.rotation_euler=(Vector((0,0,35))-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(340,-75,140));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,33))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=255
scene=bpy.context.scene;scene.camera=cam;scene.world=bpy.data.worlds.new('Studio');scene.world.use_nodes=True;next(n for n in scene.world.node_tree.nodes if n.type=='BACKGROUND').inputs[0].default_value=(.12,.13,.14,1)
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.render.resolution_x=1560;scene.render.resolution_y=650;scene.render.resolution_percentage=100;scene.view_settings.view_transform='Standard';scene.render.image_settings.file_format='PNG';scene.render.filepath=str(BASE/'Steve_compiled_preview.png')
bpy.ops.wm.save_as_mainfile(filepath=str(BASE/'Steve_compiled_review.blend'));bpy.ops.render.render(write_still=True)
