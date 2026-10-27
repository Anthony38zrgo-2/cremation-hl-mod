"""Reproducible original HOTD2 Johnny -> GoldSrc, no generated character geometry.

Inputs were decoded from the user's PC CD by GitFist/hotd2-re MIT reference
parsers, commit 7c48c8e1eafee7d275fb166b287ee0028d72ce1d. See input LICENSE.
This script deliberately does not run build_mod.py or replace other NPC assets.
"""
from pathlib import Path
import argparse, copy, hashlib, json, math, shutil, struct, subprocess, warnings
import numpy as np
from scipy.spatial.transform import Rotation
from PIL import Image

MOD = Path(__file__).resolve().parents[1]
BASE = MOD / 'assets-source/hotd2-johnny'
OUT = BASE / 'goldsrc'
DEST = MOD / 'models/cremation/hotd2/johnny'
COMPILER = MOD.parent.parent / 'Half-Life SDK/Model Tools/studiomdl.exe'
NAMES = ['Root','Torso','Head','UpperArm_R','Forearm_R','Hand_R','UpperArm_L',
         'Forearm_L','Hand_L','Pelvis','Thigh_R','Shin_R','Foot_R','Thigh_L','Shin_L','Foot_L']
# HOTD2 faces -Z; a GoldSrc world actor faces +X. Proper rotation, determinant +1.
C = np.array([[0,0,-1],[-1,0,0],[0,1,0]],float)
GROUP = [0,2,1,5,5,5,4,4,4,3,7,7,7,6,6,6]
# Deaths use zom.bin's actual falling clips; ono.bin 0x3EB-0x3EE are
# standing reactions and must not be assigned death activities.
# name, original id, GoldSrc activity, loop, ground speed, contact event
SEQS = [('idle',0x3CF,1,True,0,0),('walk',0x3FE,3,True,42,0),
        ('run',0x3C8,4,True,130,0),('attack_right',0x306,30,False,0,1),
        ('throw',0x307,31,False,0,20),('attack_left',0x308,30,False,0,2),
        ('attack_both',0x30A,31,False,0,3),('guard',0x30D,100,False,0,0),
        ('pain_small',0x3F0,26,False,0,0),('die_simple',0x3DB,36,False,0,0),
        ('die_back',0x3DA,37,False,0,0),('die_forward',0x3CC,38,False,0,0),
        ('die_violent',0x3DC,39,False,0,0),('die_head',0x3DE,66,False,0,0),
        ('pain_head',0x3F0,70,False,0,0),('pain_chest',0x3F0,71,False,0,0),
        ('pain_stomach',0x3F0,72,False,0,0),('pain_leftarm',0x3F0,73,False,0,0),
        ('pain_rightarm',0x3F0,74,False,0,0),('pain_leftleg',0x3F0,75,False,0,0),
        ('pain_rightleg',0x3F0,76,False,0,0)]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--actor-only',action='store_true',help='Rebuild/install actor and charred actor, preserving axes and gibs')
    actor_only=parser.parse_args().actor_only
    OUT.mkdir(parents=True,exist_ok=True)
    d=json.loads((BASE/'Johnny_extracted.json').read_text())
    models={m['id']:m for m in d['models']}; nodes=sorted(d['nodes'],key=lambda n:n['index'])
    parents=[-1 if n['parent'] is None else n['parent'] for n in nodes]
    motions={c['id']:c for c in d['clips']}
    def frames(mid):
        clip=motions[mid];raw=clip['frames']
        roots=np.array([f['position'] for f in raw],dtype=float)
        positions=np.tile(np.array([n['offset'] for n in nodes],dtype=float),(len(raw),1,1));positions[:,0]=roots
        angles=np.array([f['rotations'] for f in raw],dtype=float).reshape(-1,16,3)*(2*math.pi/65536)
        rotations=Rotation.from_euler('xyz',angles.reshape(-1,3)).as_matrix().reshape(-1,16,3,3)
        return positions@C.T, np.einsum('ij,fkjl,lm->fkim',C,rotations,C.T)
    pos,rot=frames(0x3CF);world_p=pos[0].copy();world_r=rot[0].copy()
    for i in range(1,16):world_p[i]=world_p[parents[i]]+world_r[parents[i]]@pos[0,i];world_r[i]=world_r[parents[i]]@rot[0,i]
    points=[]
    for n in nodes:
        if n['model_id']:
            verts=np.array(models[n['model_id']]['vertices'])[:,:3]@C.T
            points.extend(verts@world_r[n['index']].T+world_p[n['index']])
    points=np.array(points);scale=72/(points[:,2].max()-points[:,2].min());floor=-points[:,2].min()*scale
    local_ref=np.array([n['offset'] for n in nodes])@C.T*scale;local_ref[0]=[0,0,floor]
    rest=local_ref.copy()
    for i in range(1,16):rest[i]+=rest[parents[i]]
    node_text='version 1\nnodes\n'+''.join(f'{i} "{name}" {parents[i]}\n' for i,name in enumerate(NAMES))+'end\n'
    reference=node_text+'skeleton\ntime 0\n'+''.join(f'{i} '+' '.join(f'{v:.7f}' for v in (*local_ref[i],0,0,0))+'\n' for i in range(16))+'end\n'
    # Bake native material tint, alpha usage and mirrored addressing into GoldSrc textures.
    textures=[];material_names={};material_keys={};uv_div={}
    for model in d['models']:
        for mi,mat in enumerate(model['materials']):
            words=mat['words'];ti=words[8];tsp=words[2]
            diffuse=tuple(struct.unpack('<f',struct.pack('<I',v))[0] for v in words[12:15])
            masked=(tsp&0x180000)!=0x80000
            mirror_u=bool(tsp&0x40000) and not bool(tsp&0x10000)
            mirror_v=bool(tsp&0x20000) and not bool(tsp&0x8000)
            key=(ti,diffuse,masked,mirror_u,mirror_v)
            if key not in material_keys:
                if ti<len(d['textures']):im=Image.open(BASE/d['textures'][ti]['path']).convert('RGBA')
                else:im=Image.new('RGBA',(16,16),(255,255,255,255))
                rgba=np.array(im);rgb=np.clip(np.round(rgba[:,:,:3].astype(float)*np.array(diffuse)),0,255).astype('uint8');alpha=rgba[:,:,3] if masked else np.full(rgba.shape[:2],255,dtype='uint8')
                if mirror_u:rgb=np.concatenate([rgb,rgb[:,::-1]],axis=1);alpha=np.concatenate([alpha,alpha[:,::-1]],axis=1)
                # Exported PNG/BMP rows are vertically flipped relative to the source.
                if mirror_v:rgb=np.concatenate([rgb[::-1],rgb],axis=0);alpha=np.concatenate([alpha[::-1],alpha],axis=0)
                if ti==17 and masked:
                    # GoldSrc has one-bit texture alpha. Keep the source's graded
                    # hair coverage at 4x resolution instead of discarding faint strands.
                    rgb=np.repeat(np.repeat(rgb,4,axis=0),4,axis=1);alpha=np.repeat(np.repeat(alpha,4,axis=0),4,axis=1)
                    bayer=np.array([[0,8,2,10],[12,4,14,6],[3,11,1,9],[15,7,13,5]],dtype=float)
                    threshold=np.tile((bayer+.5)*255/16,(alpha.shape[0]//4,alpha.shape[1]//4))
                    alpha=np.where(alpha>threshold,255,0).astype('uint8')
                rgb=Image.fromarray(rgb);index=len(textures)
                if masked and alpha.min()<128:
                    q=rgb.quantize(colors=255);palette=q.getpalette()[:765]+[0,0,255];pix=np.array(q);pix[alpha<128]=255;q=Image.fromarray(pix,'P');q.putpalette(palette);name=f'{{johnny_{index:02d}.bmp'
                else:q=rgb.quantize(colors=256);name=f'johnny_{index:02d}.bmp'
                q.save(OUT/name);textures.append(name);material_keys[key]=name
            material_names[(model['id'],mi)]=material_keys[key];uv_div[(model['id'],mi)]=(2 if mirror_u else 1,2 if mirror_v else 1)
    def without_axes(model):
        keep=[i for i,mi in enumerate(model['face_materials']) if model['materials'][mi]['words'][8] not in (15,16)]
        clone=copy.deepcopy(model);clone['faces']=[model['faces'][i] for i in keep];clone['face_materials']=[model['face_materials'][i] for i in keep]
        return clone
    def axe_only(model):
        keep=[i for i,mi in enumerate(model['face_materials']) if model['materials'][mi]['words'][8] in (15,16)]
        clone=copy.deepcopy(model);clone['faces']=[model['faces'][i] for i in keep];clone['face_materials']=[model['face_materials'][i] for i in keep]
        return clone
    noaxe={5:without_axes(models[nodes[5]['model_id']]),8:without_axes(models[nodes[8]['model_id']])}
    axes={5:axe_only(models[nodes[5]['model_id']]),8:axe_only(models[nodes[8]['model_id']])}
    assert all(len(axes[n]['faces'])>0 for n in (5,8)) and all(len(noaxe[n]['faces'])>0 for n in (5,8))
    def mesh_smd(m,bone=None,gib=False):
        verts=[]
        for vi,v in enumerate(m['vertices']):
            bi=bone;vv=np.array(v[:3]);nn=np.array(v[3:6])
            if bone is None:
                a=d['attachments'][0]['assignments'].get(str(vi));bi=a['bone'] if a else 9
                if a:vv=np.array(a['position']);nn=np.array(a['normal'])
            pt=C@vv*scale+(np.zeros(3) if gib else rest[bi]);nr=C@nn
            verts.append((0 if gib else bi,*pt,*nr,*v[6:8]))
        text=('version 1\nnodes\n0 "piece" -1\nend\nskeleton\ntime 0\n0 0 0 0 0 0 0\nend\n' if gib else reference)+'triangles\n'
        for face,mi in zip(m['faces'],m['face_materials']):
            text+=material_names[(m['id'],mi)]+'\n'
            du,dv=uv_div[(m['id'],mi)]
            # This compiler reverses SMD triangles when building strip commands;
            # the decoded source already has outward CCW faces. Readback checks
            # the final GoldSrc clockwise convention rather than double-flipping.
            for vi in face:
                v=list(verts[vi]);v[-2]/=du;v[-1]/=dv;text+=str(v[0])+' '+' '.join(f'{x:.7f}' for x in v[1:])+'\n'
        return text+'end\n'
    counts=[];bases=[];base=1;bodygroups=[];hitboxes=[];gibfiles=[];part_bounds=[[[0]*6 for _ in range(5)]]
    qc=f'$modelname "{(OUT/"johnny.mdl").as_posix()}"\n$cd "{OUT.as_posix()}"\n$cdtexture "{OUT.as_posix()}"\n$origin 0 0 0 -90\n$eyeposition 0 0 64\n$bbox -16 -16 0 16 16 72\n$attachment 0 "Head" 0 0 0\n'
    for n in nodes[1:]:
        i=n['index'];mid=n['model_id'];variants=[mid]
        if i==9:variants=[mid]
        elif i in (12,15):variants=[mid,None]
        elif i in (5,8):variants=[mid,mid,mid,mid,None]
        else:variants=[mid,mid,mid,None]
        bases.append(base);counts.append(len(variants));base*=len(variants);assert base<2**31
        qc+=f'$bodygroup {NAMES[i]}\n{{\n'
        for state,vid in enumerate(variants):
            if vid is None:qc+=' blank\n';continue
            name=f'part_{i:02d}_{state}';mesh=models[vid] if not (i in (5,8) and state==3) else noaxe[i];(OUT/(name+'.smd')).write_text(mesh_smd(mesh,i));qc+=f' studio "{name}"\n'
        qc+='}\n';bodygroups.append(dict(node=i,base=bases[-1],count=len(variants),models=variants))
        vv=np.array(models[mid]['vertices'])[:,:3]@C.T*scale;lo=vv.min(0)-.4;hi=vv.max(0)+.4
        if i==9:
            # Abdomen is a separate deformable attachment. Its broad anatomical
            # region follows the pelvis and must not leave an unhittable waist.
            am=models[d['attachments'][0]['model_id']];ap=[]
            for vi,v in enumerate(am['vertices']):
                a=d['attachments'][0]['assignments'][str(vi)];ap.append(C@np.array(a['position'])*scale+rest[a['bone']]-rest[9])
            lo=np.minimum(lo,np.min(ap,axis=0)-.4);hi=np.maximum(hi,np.max(ap,axis=0)+.4)
        bounds=[]
        for state,vid in enumerate(variants):
            if vid is None:bounds.append([0]*6)
            else:
                bm=noaxe[i] if i in (5,8) and state==3 else models[vid]
                pv=np.array(bm['vertices'])[:,:3]@C.T*scale
                low=np.minimum(pv.min(0)-.4,lo) if i==9 else pv.min(0)-.4
                high=np.maximum(pv.max(0)+.4,hi) if i==9 else pv.max(0)+.4
                bounds.append([*low,*high])
        while len(bounds)<5:bounds.append([0]*6)
        part_bounds.append(bounds)
        qc+=f'$hbox {GROUP[i]} "{NAMES[i]}" '+' '.join(f'{x:.5f}' for x in (*lo,*hi))+'\n'
        hitboxes.append(dict(node=i,group=GROUP[i],minimum=lo.tolist(),maximum=hi.tolist()))
        if i not in (1,9):
            gqc=f'$modelname "{(OUT/f"part_{i:02d}.mdl").as_posix()}"\n$cd "{OUT.as_posix()}"\n$cdtexture "{OUT.as_posix()}"\n$origin 0 0 0 -90\n$bodygroup piece\n{{\n'
            for state,vid in enumerate(variants):
                if vid is None:continue
                name=f'gib_{i:02d}_{state}';mesh=models[vid] if not (i in (5,8) and state==3) else noaxe[i];(OUT/(name+'.smd')).write_text(mesh_smd(mesh,i,True));gqc+=f' studio "{name}"\n'
            gqc+='}\n$sequence idle "piece_idle" fps 1\n';(OUT/f'part_{i:02d}.qc').write_text(gqc);gibfiles.append(f'part_{i:02d}')
    (OUT/'abdomen.smd').write_text(mesh_smd(models[d['attachments'][0]['model_id']]))
    qc+='$body abdomen "abdomen"\n'
    for node,stem in ((5,'axe'),(8,'axe_left')):
        (OUT/(stem+'.smd')).write_text(mesh_smd(axes[node],node,True))
        (OUT/(stem+'.qc')).write_text(f'$modelname "{(OUT/(stem+".mdl")).as_posix()}"\n$cd "{OUT.as_posix()}"\n$cdtexture "{OUT.as_posix()}"\n$origin 0 0 0 -90\n$bodygroup axe\n{{\n studio "{stem}"\n}}\n$sequence idle "piece_idle" fps 1\n')
    # Piece bounds and bind matrix are compiled in exactly the same coordinate basis as actor bones.
    (OUT/'piece_idle.smd').write_text('version 1\nnodes\n0 "piece" -1\nend\nskeleton\ntime 0\n0 0 0 0 0 0 0\nend\n')
    seq_report=[]
    for name,mid,activity,loop,speed,event in SEQS:
        pos,rot=frames(mid)
        pos*=scale;pos[:,0,2]+=floor
        # Remove root travel before anchoring the visible lower body.
        root_delta=pos[-1,0,:2]-pos[0,0,:2];pos[:,0,:2]=pos[0,0,:2]
        # Entity navigation owns translation. Retain authored vertical motion but remove root travel.
        # Export root motion only into QC LX extraction so StudioMDL computes native ground speed.
        if speed:pos[:,0,0]=pos[0,0,0]+np.linspace(0,speed*(len(pos)-1)/30,len(pos))
        # StudioMDL forces a loop's final pose to its first. Make that explicit
        # in the SMD so readback and original-to-GoldSrc closure are auditable.
        if loop and len(pos)>1:
            pos[-1,1:]=pos[0,1:];pos[-1,0,1:]=pos[0,0,1:];rot[-1]=rot[0]
        if name.startswith('die_'):
            final_pos=pos[-1].copy();final_rot=rot[-1].copy()
            for node in range(1,16):
                parent=parents[node]
                final_pos[node]=final_pos[parent]+final_rot[parent]@final_pos[node]
                final_rot[node]=final_rot[parent]@final_rot[node]
            if final_pos[2,2]>=30 or final_pos[9,2]>=25:
                raise RuntimeError(f'{name} 0x{mid:X} ends standing: head={final_pos[2,2]:.2f}, pelvis={final_pos[9,2]:.2f}')
        with warnings.catch_warnings():
            warnings.simplefilter('ignore');angles=Rotation.from_matrix(rot.reshape(-1,3,3)).as_euler('xyz').reshape(*rot.shape[:2],3)
        smd=node_text+'skeleton\n'
        for f in range(len(pos)):
            smd+=f'time {f}\n'+''.join(f'{i} '+' '.join(f'{x:.7f}' for x in (*pos[f,i],*angles[f,i]))+'\n' for i in range(16))
        (OUT/(name+'.smd')).write_text(smd+'end\n')
        # Contact events are placed on the visible swing/release phase of Johnny's original clips.
        event_frame={0x306:10,0x307:18,0x308:27,0x30A:29}.get(mid,0) if event else 0
        if event:assert 0<event_frame<len(pos)
        qc+=f'$sequence {name} "{name}" fps 30'+(' loop' if loop else '')+(' LX' if speed else '')
        if event:qc+=' { event '+str(event)+' '+str(event_frame)+' }'
        qc+='\n';seq_report.append(dict(name=name,source_id=mid,frames=len(pos),activity=activity,loop=loop,speed=speed,event=event,event_frame=event_frame,discarded_root_travel=root_delta.tolist()))
    (OUT/'johnny.qc').write_text(qc)
    for stem in (['johnny'] if actor_only else ['johnny','axe','axe_left',*gibfiles]):
        r=subprocess.run([str(COMPILER),str(OUT/(stem+'.qc'))],capture_output=True,text=True)
        (OUT/(stem+'_compile.log')).write_text(r.stdout+r.stderr)
        if r.returncode or not (OUT/(stem+'.mdl')).exists():raise RuntimeError(r.stdout+r.stderr)
    data=bytearray((OUT/'johnny.mdl').read_bytes());assert data[:4]==b'IDST'
    for paxe in ('axe.mdl','axe_left.mdl'):assert (OUT/paxe).read_bytes()[:4]==b'IDST'
    seqoff=struct.unpack_from('<i',data,168)[0]
    for i,seq in enumerate(seq_report):struct.pack_into('<ii',data,seqoff+i*176+40,seq['activity'],1)
    nt,to=struct.unpack_from('<ii',data,180)
    for i in range(nt):
        at=to+i*80
        if data[at:at+1]==b'{':struct.pack_into('<i',data,at+64,struct.unpack_from('<i',data,at+64)[0]|64)
    (OUT/'johnny.mdl').write_bytes(data)
    # Gib models use the same masked textures and this old compiler likewise
    # needs explicit STUDIO_NF_MASKED on the final texture records.
    for path in ([] if actor_only else [OUT/'axe.mdl',OUT/'axe_left.mdl']+[OUT/(stem+'.mdl') for stem in gibfiles]):
        g=bytearray(path.read_bytes());gn,gt=struct.unpack_from('<ii',g,180)
        for i in range(gn):
            at=gt+i*80
            if g[at:at+1]==b'{':struct.pack_into('<i',g,at+64,struct.unpack_from('<i',g,at+64)[0]|64)
        path.write_bytes(g)
    charred=bytearray(data);nt,to=struct.unpack_from('<ii',data,180)
    for i in range(nt):
        w,h,pixels=struct.unpack_from('<iii',data,to+i*80+68);pal=pixels+w*h
        charred[pal:pal+768]=bytes(round(c*.73) for c in data[pal:pal+768])
    (OUT/'johnny_charred.mdl').write_bytes(charred)
    # StudioMDL rotates source 90 degrees with the compensating QC -90: verify the compiled bones.
    nb,bo=struct.unpack_from('<ii',data,140);assert nb==16
    compiled={}
    for i in range(nb):
        at=bo+i*112;name=bytes(data[at:at+32]).split(b'\0')[0].decode();compiled[name]=i
    assert set(compiled)==set(NAMES)
    report=dict(source='HOTD2 PC Johnny kind 0x13; geometry F12992_znonoo.bin; textures F13793_znonoo.bin; motions F12583_zom.bin/F12597_ono.bin',scale=scale,floor=floor,bones=compiled,bodygroups=bodygroups,hitboxes=hitboxes,sequences=seq_report,textures=len(textures),main_triangles=sum(len(models[n['model_id']]['faces']) for n in nodes if n['model_id']),axe_projectile_triangles={str(n):len(axes[n]['faces']) for n in (5,8)},sha256={})
    DEST.mkdir(parents=True,exist_ok=True)
    backup=MOD/'backups/before-hotd2-johnny';backup.mkdir(parents=True,exist_ok=True)
    install_paths=[OUT/'johnny.mdl',OUT/'johnny_charred.mdl']
    if not actor_only:install_paths += [OUT/'axe.mdl',OUT/'axe_left.mdl',*[OUT/(stem+'.mdl') for stem in gibfiles]]
    for path in install_paths:
        dst=DEST/path.name
        if dst.exists() and not (backup/path.name).exists():shutil.copy2(dst,backup/path.name)
        shutil.copy2(path,dst);report['sha256'][dst.relative_to(MOD).as_posix()]=hashlib.sha256(path.read_bytes()).hexdigest()
    (BASE/'conversion-report.json').write_text(json.dumps(report,indent=2)+'\n')
    # Constants come from the compiled file, rather than assuming bone/body ordering.
    header='// Generated by tools/convert_hotd2_johnny.py.\n#pragma once\nnamespace johnny_assets {\n'
    header+='static constexpr int bone[16]={'+','.join(str(compiled[n]) for n in NAMES)+'};\n'
    seq_index={entry[0]:i for i,entry in enumerate(SEQS)}
    for name,constant in [('idle','seqIdle'),('walk','seqWalk'),('run','seqRun'),('attack_right','seqAttackRight'),('throw','seqThrow'),('attack_left','seqAttackLeft'),('attack_both','seqAttackBoth'),('guard','seqGuard'),('pain_small','seqPain'),('die_simple','seqDieSimple'),('die_back','seqDieBack'),('die_forward','seqDieForward'),('die_violent','seqDieViolent')]:
        header+=f'static constexpr int {constant}={seq_index[name]};\n'
    header+='static constexpr int parent[16]={'+','.join(map(str,parents))+'};\n'
    header+='static constexpr int bodyBase[16]={0,'+','.join(map(str,bases))+'};\n'
    header+='static constexpr int bodyCount[16]={1,'+','.join(map(str,counts))+'};\n'
    header+='static constexpr int hitgroup[16]={'+','.join(map(str,GROUP))+'};\n'
    header+='static constexpr float hitbox[16][6]={{0,0,0,0,0,0},'+','.join('{'+','.join(f'{x:.6f}f' for x in (*h['minimum'],*h['maximum']))+'}' for h in hitboxes)+'};\n'
    header+='static constexpr float partBounds[16][5][6]={'+','.join('{'+','.join('{'+','.join(f'{x:.6f}f' for x in box)+'}' for box in part)+'}' for part in part_bounds)+'};\n'
    header+='static constexpr float duration['+str(len(seq_report))+']={'+','.join(f'{max(1,s["frames"]-1)/30:.7f}f' for s in seq_report)+'};\n'
    header+='}\n';(MOD/'source/johnny_assets.h').write_text(header)
    print(f'Johnny compiled/installed: {nb} bones, {len(textures)} textures, {len(seq_report)} sequences; '+('actor and charred actor only' if actor_only else f'two axe states, {len(gibfiles)} regional gib models'))

if __name__=='__main__':main()
