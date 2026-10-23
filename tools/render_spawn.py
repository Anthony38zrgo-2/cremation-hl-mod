"""Offline geometry reference; does not launch or control the game."""
import bpy, sys, math, struct
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).parent))
from bsp import BSP
root=Path(__file__).resolve().parents[2]
b=BSP(root/'valve/maps/rapidcore.bsp')
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
verts=b.records(3,'<3f'); edges=b.records(12,'<2H'); ses=[s[0] for s in b.records(13,'<i')]
faces=b.records(7,'<Hhihh4Bi'); infos=b.records(6,'<8f2i')
tex=b.lump(2); n=struct.unpack_from('<i',tex)[0]
wads={}
for path in (root/'valve').glob('*.wad'):
    data=path.read_bytes()
    if data[:4]!=b'WAD3': continue
    count,offset=struct.unpack_from('<ii',data,4)
    for i in range(count):
        pos,size,_,kind,_,_,name=struct.unpack_from('<iiiBBH16s',data,offset+i*32)
        if kind==67: wads[name.split(b'\0')[0].lower()]=data[pos:pos+size]
materials=[]
for i in range(n):
    offset=struct.unpack_from('<i',tex,4+i*4)[0]
    if offset<0: materials.append(None); continue
    chunk=tex[offset:]; name,w,h,mip=struct.unpack_from('<16sIII',chunk)
    name=name.split(b'\0')[0].decode('latin1')
    if mip==0:
        chunk=wads.get(name.lower().encode(),chunk)
        _,w,h,mip=struct.unpack_from('<16sIII',chunk)
    mat=bpy.data.materials.new(name); mat.use_nodes=True
    nodes=mat.node_tree.nodes; nodes.clear()
    out=nodes.new('ShaderNodeOutputMaterial'); shader=nodes.new('ShaderNodeEmission')
    shader.inputs[0].default_value=(.25,.3,.35,1)
    mat.node_tree.links.new(shader.outputs[0],out.inputs[0])
    if mip:
        offsets=struct.unpack_from('<4I',chunk,24)
        palette_offset=offsets[3]+(w//8)*(h//8)+2
        palette=chunk[palette_offset:palette_offset+768]
        if len(palette)==768:
            indices=chunk[mip:mip+w*h]
            pixels=[]
            for y in range(h-1,-1,-1):
                for x in range(w):
                    idx=indices[y*w+x]*3
                    pixels.extend([palette[idx]/255,palette[idx+1]/255,palette[idx+2]/255,1])
            image=bpy.data.images.new(name,width=w,height=h); image.pixels.foreach_set(pixels)
            texture=nodes.new('ShaderNodeTexImage'); texture.image=image; texture.interpolation='Closest'
            mat.node_tree.links.new(texture.outputs['Color'],shader.inputs[0])
    materials.append((mat,w,h,name))
model_ids={0}|{int(e['model'][1:]) for e in b.entities if e.get('model','').startswith('*') and e.get('classname') not in ('trigger_hurt','trigger_gravity','func_water','func_ladder')}
polys=[]; uvcoords=[]; mids=[]
for mi in model_ids:
    model=b.models[mi]
    for face in faces[model[14]:model[14]+model[15]]:
        info=infos[face[4]]; ti=info[8]; mat=materials[ti]
        if not mat or mat[3].lower() in ('sky','clip','origin','aaatrigger') or mat[3].startswith('!'): continue
        poly=[edges[abs(se)][0 if se>=0 else 1] for se in ses[face[2]:face[2]+face[3]]]
        polys.append(poly); mids.append(ti)
        uvcoords.extend([(sum(verts[v][j]*info[j] for j in range(3))+info[3])/mat[1], (sum(verts[v][j]*info[j+4] for j in range(3))+info[7])/mat[2]] for v in poly)
mesh=bpy.data.meshes.new('rapidcore'); mesh.from_pydata(verts,[],polys); mesh.update()
obj=bpy.data.objects.new('rapidcore',mesh); bpy.context.collection.objects.link(obj)
for mat in materials: mesh.materials.append(mat[0] if mat else bpy.data.materials.new('missing'))
for polygon,mi in zip(mesh.polygons,mids): polygon.material_index=mi
uv=mesh.uv_layers.new()
for loop,p in zip(uv.data,uvcoords): loop.uv=p
camdata=bpy.data.cameras.new('spawn'); cam=bpy.data.objects.new('spawn',camdata); bpy.context.collection.objects.link(cam)
cam.location=(432,-1601,-192); yaw=math.radians(155)
cam.rotation_euler=Vector((math.cos(yaw),math.sin(yaw),0)).to_track_quat('-Z','Y').to_euler()
camdata.type='PERSP'; camdata.lens=13.5; camdata.sensor_width=36; camdata.clip_start=.1; camdata.clip_end=6000
scene=bpy.context.scene; scene.camera=cam; scene.render.engine='CYCLES'; scene.cycles.samples=1
scene.render.resolution_x=1280; scene.render.resolution_y=720; scene.render.resolution_percentage=100
scene.view_settings.view_transform='Standard'
scene.render.filepath=str(root/'Cremation/tools/spawn-reference.png')
bpy.ops.render.render(write_still=True)
