from pathlib import Path
import bpy,json,math
from mathutils import Vector
BASE=Path(__file__).resolve().parents[1]/'assets-source/hotd2-steve'
before=json.loads((BASE/'head-before.json').read_text());after=json.loads((BASE/'compiled-preview.json').read_text());p=after['poses'][0];after['poses']=[{**p,'faces':p['faces'][78:248],'uv':p['uv'][78:248],'materials':p['materials'][78:248],'name':'Corregido'}]
bpy.ops.wm.read_factory_settings(use_empty=True)
for side,data in enumerate((before,after)):
 mats=[]
 for i,path in enumerate(data['textures']):
  mat=bpy.data.materials.new(f'{side}_{i}');mat.use_nodes=True;mat.use_backface_culling=True;bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=1;bs.inputs['Specular IOR Level'].default_value=0
  tx=mat.node_tree.nodes.new('ShaderNodeTexImage');tx.image=bpy.data.images.load(path);tx.interpolation='Closest';tx.image.pack()
  for output,inp in [('Color','Base Color'),('Color','Emission Color'),('Alpha','Alpha')]:mat.node_tree.links.new(tx.outputs[output],bs.inputs[inp])
  bs.inputs['Emission Strength'].default_value=.3;mats.append(mat)
 p=data['poses'][0];used=sorted({i for f in p['faces'] for i in f});index={old:new for new,old in enumerate(used)};verts=[p['vertices'][i] for i in used];faces=[[index[i] for i in reversed(f)] for f in p['faces']]
 me=bpy.data.meshes.new(p['name']);me.from_pydata(verts,[],faces);me.update();ob=bpy.data.objects.new(p['name'],me);bpy.context.collection.objects.link(ob);ob.location=Vector((13,100,0)).normalized()*((side-.5)*17)
 for mat in mats:me.materials.append(mat)
 uv=me.uv_layers.new()
 for poly,coords,mi in zip(me.polygons,p['uv'],p['materials']):
  poly.material_index=mi;poly.use_smooth=True
  for li,co in zip(poly.loop_indices,reversed(coords)):uv.data[li].uv=co
 center=Vector(tuple(sum(v[k] for v in verts)/len(verts) for k in range(3)));zmin=min(v[2] for v in verts)
 bpy.ops.object.text_add(location=Vector((14,0,zmin-2.5))+ob.location);label=bpy.context.object;label.data.body=p['name'];label.data.align_x='CENTER';label.data.size=1.8;label.rotation_euler=(math.pi/2,0,math.pi/2)
center.y=0
for loc,energy in [((60,-40,120),100000),((40,60,90),70000),((-30,0,110),40000)]:
 bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=energy;o.data.size=60;o.rotation_euler=(center-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=center+Vector((100,-13,20)));cam=bpy.context.object;cam.rotation_euler=(center+Vector((0,0,-.5))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=36
scene=bpy.context.scene;scene.camera=cam;scene.world=bpy.data.worlds.new('Studio');scene.world.color=(.15,.15,.15);scene.render.engine='CYCLES';scene.cycles.samples=32;scene.render.resolution_x=1200;scene.render.resolution_y=700;scene.render.image_settings.file_format='PNG';scene.render.filepath=str(BASE/'Steve_hair_comparison.png');scene.view_settings.view_transform='Standard'
bpy.ops.wm.save_as_mainfile(filepath=str(BASE/'Steve_hair_review.blend'));bpy.ops.render.render(write_still=True)
