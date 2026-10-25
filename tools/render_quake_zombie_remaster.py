"""Blender background QA of the actual compiled GoldSrc before/after meshes."""
from pathlib import Path
import bpy, json, math
from mathutils import Vector

BASE=Path(__file__).resolve().parents[1]/'assets-source/quake-zombie-remaster'
data=json.loads((BASE/'render-preview.json').read_text())
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
for i,entry in enumerate(data):
    mesh=bpy.data.meshes.new(entry['name']);mesh.from_pydata(entry['vertices'],[],entry['faces']);mesh.update()
    obj=bpy.data.objects.new(entry['name'],mesh);bpy.context.collection.objects.link(obj);obj.location.y=(i-.5)*105
    layer=mesh.uv_layers.new(name='UVMap')
    for face,uv in zip(mesh.polygons,entry['uv']):
        for loop,coord in zip(face.loop_indices,uv):layer.data[loop].uv=coord
        face.use_smooth=True
    material=bpy.data.materials.new(entry['name']);material.use_nodes=True
    shader=material.node_tree.nodes.get('Principled BSDF');shader.inputs['Roughness'].default_value=.85
    tex=material.node_tree.nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(entry['texture']);tex.interpolation='Linear'
    material.node_tree.links.new(tex.outputs['Color'],shader.inputs['Base Color']);obj.data.materials.append(material)

bpy.ops.mesh.primitive_plane_add(size=2000,location=(0,0,-.8));ground=bpy.context.object
mat=bpy.data.materials.new('Floor');mat.diffuse_color=(.15,.17,.19,1);ground.data.materials.append(mat)
world=bpy.context.scene.world;world.color=(.35,.35,.35)
for loc,power,size in [((160,-220,300),2200000,200),((100,220,180),1600000,180),((-200,0,220),1300000,160)]:
    bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=size;light.rotation_euler=(Vector((0,0,65))-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(360,-175,180));camera=bpy.context.object;camera.rotation_euler=(Vector((0,0,66))-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=230
scene=bpy.context.scene;scene.camera=camera
for y,label in [(-52.5,'SGClassic: 50 u'),(52.5,'Remaster: 129 u / 2.5x')]:
    bpy.ops.object.text_add(location=(35,y,-12));t=bpy.context.object;t.data.body=label;t.data.align_x='CENTER';t.data.size=6;t.rotation_euler=camera.rotation_euler
    textmat=bpy.data.materials.new(label);textmat.diffuse_color=(.9,.93,.97,1);t.data.materials.append(textmat)
scene.render.engine='CYCLES';scene.cycles.samples=48
scene.render.resolution_x=1100;scene.render.resolution_y=800;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(BASE/'comparison.png')
scene.view_settings.view_transform='AgX'
bpy.ops.wm.save_as_mainfile(filepath=str(BASE/'comparison.blend'))
bpy.ops.render.render(write_still=True)
