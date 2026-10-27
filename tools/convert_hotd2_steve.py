"""Reproducible original HOTD2 Steve -> GoldSrc, no generated character geometry.

Inputs were decoded from the user's PC CD by GitFist/hotd2-re MIT reference
parsers, commit 7c48c8e1eafee7d275fb166b287ee0028d72ce1d. See input LICENSE.
This script deliberately does not run build_mod.py or replace other NPC assets.
"""
from pathlib import Path
import copy, hashlib, json, math, shutil, struct, subprocess, warnings
import numpy as np
from scipy.spatial.transform import Rotation
from PIL import Image

MOD = Path(__file__).resolve().parents[1]
BASE = MOD / 'assets-source/hotd2-steve'
OUT = BASE / 'goldsrc'
DEST = MOD / 'models/cremation/hotd2/steve'
COMPILER = MOD.parent.parent / 'Half-Life SDK/Model Tools/studiomdl.exe'
NAMES = ['Root','Torso','Head','UpperArm_R','Forearm_R','Hand_R','UpperArm_L',
         'Forearm_L','Hand_L','Pelvis','Thigh_R','Shin_R','Foot_R','Thigh_L','Shin_L','Foot_L']
# HOTD2 faces -Z; a GoldSrc world actor faces +X. Proper rotation, determinant +1.
C = np.array([[0,0,-1],[-1,0,0],[0,1,0]],float)
GROUP = [0,2,1,5,5,5,4,4,4,3,7,7,7,6,6,6]
# Source motion IDs checked against the original skeleton pose catalog.
# name, original id, GoldSrc activity, loop, ground speed, contact event
SEQS = [('idle',0x3CF,1,True,0,0),('walk',0x3FE,3,True,42,0),
        ('run',0x3C8,4,True,130,0),('attack_right',0x3EA,30,False,0,1),
        ('attack_left',0x3E9,30,False,0,2),('attack_both',0x3C9,31,False,0,3),
        ('pain_small',0x3D6,26,False,0,0),('pain_big',0x3CE,27,False,0,0),
        ('die_simple',0x3DB,36,False,0,0),('die_back',0x3DA,37,False,0,0),
        ('die_forward',0x3CC,38,False,0,0),('die_violent',0x3DC,39,False,0,0),
        ('die_head',0x3DE,66,False,0,0),('pain_head',0x3D1,70,False,0,0),
        ('pain_chest',0x3D6,71,False,0,0),('pain_stomach',0x3CE,72,False,0,0),
        ('pain_leftarm',0x3D3,73,False,0,0),('pain_rightarm',0x3D5,74,False,0,0),
        ('pain_leftleg',0x3C0,75,False,0,0),('pain_rightleg',0x3C1,76,False,0,0),
        ('crawl',0x41B,48,True,16,0),('crawl_idle',0x41E,18,True,0,0),
        ('prone_attack_right',0x3F4,30,False,0,1),('prone_attack_left',0x3F5,30,False,0,2),
        ('prone_attack_both',0x3F5,30,False,0,3),('prone_die',0x404,36,False,0,0)]

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    d=json.loads((BASE/'Steve_extracted.json').read_text())
    models={m['id']:m for m in d['models']}; nodes=sorted(d['nodes'],key=lambda n:n['index'])
    parents=[-1 if n['parent'] is None else n['parent'] for n in nodes]
    motion=(BASE/'zom.bin').read_bytes();table=json.loads((BASE/'motion_table.json').read_text())
    offsets=list(struct.unpack_from(f'<{len(table)}I',motion))+[len(motion)]
    motions={t['id']:(motion[offsets[i]:offsets[i+1]],t['ticks']) for i,t in enumerate(table)}
    crawler=(BASE/'hzom.bin').read_bytes()
    crawler_table=json.loads((BASE/'hzom_motion_table.json').read_text())
    crawler_offsets=list(struct.unpack_from(f'<{len(crawler_table)}I',crawler))+[len(crawler)]
    motions.update({t['id']:(crawler[crawler_offsets[i]:crawler_offsets[i+1]],t['ticks']) for i,t in enumerate(crawler_table)})
    def frames(mid):
        block,ticks=motions[mid];nf=(ticks-1)//2+2;stride=108
        assert len(block)>=4+nf*stride
        positions=[];rotations=[]
        for f in range(nf):
            p0=struct.unpack_from('<3f',block,4+f*stride)
            angles=np.array(struct.unpack_from('<48h',block,16+f*stride)).reshape(16,3)*2*math.pi/65536
            r=Rotation.from_euler('xyz',angles).as_matrix()
            local=np.array([n['offset'] for n in nodes]);local[0]=p0
            positions.append(local@C.T);rotations.append(C@r@C.T)
        return np.array(positions),np.array(rotations)
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
                if ti==8 and masked:
                    # GoldSrc has one-bit texture alpha. Keep the source's graded
                    # hair coverage at 4x resolution instead of discarding faint strands.
                    rgb=np.repeat(np.repeat(rgb,4,axis=0),4,axis=1);alpha=np.repeat(np.repeat(alpha,4,axis=0),4,axis=1)
                    bayer=np.array([[0,8,2,10],[12,4,14,6],[3,11,1,9],[15,7,13,5]],dtype=float)
                    threshold=np.tile((bayer+.5)*255/16,(alpha.shape[0]//4,alpha.shape[1]//4))
                    alpha=np.where(alpha>threshold,255,0).astype('uint8')
                rgb=Image.fromarray(rgb);index=len(textures)
                if masked and alpha.min()<128:
                    q=rgb.quantize(colors=255);palette=q.getpalette()[:765]+[0,0,255];pix=np.array(q);pix[alpha<128]=255;q=Image.fromarray(pix,'P');q.putpalette(palette);name=f'{{steve_{index:02d}.bmp'
                else:q=rgb.quantize(colors=256);name=f'steve_{index:02d}.bmp'
                q.save(OUT/name);textures.append(name);material_keys[key]=name
            material_names[(model['id'],mi)]=material_keys[key];uv_div[(model['id'],mi)]=(2 if mirror_u else 1,2 if mirror_v else 1)
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
    counts=[];bases=[];base=1;bodygroups=[];hitboxes=[];gibfiles=[];part_bounds=[[[0]*6 for _ in range(4)]]
    qc=f'$modelname "{(OUT/"steve.mdl").as_posix()}"\n$cd "{OUT.as_posix()}"\n$cdtexture "{OUT.as_posix()}"\n$origin 0 0 0 -90\n$eyeposition 0 0 64\n$bbox -16 -16 0 16 16 72\n$attachment 0 "Head" 0 0 0\n'
    for n in nodes[1:]:
        i=n['index'];mid=n['model_id'];variants=[mid]
        if i==1:variants=[mid,mid+1,mid+2,mid+3]
        elif i==2:variants=[mid,8017,8018,None]
        elif i in (10,11,13,14):variants=[mid,mid+1,mid+1,None]
        elif i not in (9,12,15):variants=[mid,mid+1,mid+2,None]
        elif i in (12,15):variants=[mid,None]
        bases.append(base);counts.append(len(variants));base*=len(variants);assert base<2**31
        qc+=f'$bodygroup {NAMES[i]}\n{{\n'
        for state,vid in enumerate(variants):
            if vid is None:qc+=' blank\n';continue
            name=f'part_{i:02d}_{state}';(OUT/(name+'.smd')).write_text(mesh_smd(models[vid],i));qc+=f' studio "{name}"\n'
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
        for vid in variants:
            if vid is None:bounds.append([0]*6)
            else:
                pv=np.array(models[vid]['vertices'])[:,:3]@C.T*scale
                low=np.minimum(pv.min(0)-.4,lo) if i==9 else pv.min(0)-.4
                high=np.maximum(pv.max(0)+.4,hi) if i==9 else pv.max(0)+.4
                bounds.append([*low,*high])
        while len(bounds)<4:bounds.append([0]*6)
        part_bounds.append(bounds)
        qc+=f'$hbox {GROUP[i]} "{NAMES[i]}" '+' '.join(f'{x:.5f}' for x in (*lo,*hi))+'\n'
        hitboxes.append(dict(node=i,group=GROUP[i],minimum=lo.tolist(),maximum=hi.tolist()))
        if i not in (1,9):
            gqc=f'$modelname "{(OUT/f"part_{i:02d}.mdl").as_posix()}"\n$cd "{OUT.as_posix()}"\n$cdtexture "{OUT.as_posix()}"\n$origin 0 0 0 -90\n$bodygroup piece\n{{\n'
            for state,vid in enumerate(variants):
                if vid is None:continue
                name=f'gib_{i:02d}_{state}';(OUT/(name+'.smd')).write_text(mesh_smd(models[vid],i,True));gqc+=f' studio "{name}"\n'
            gqc+='}\n$sequence idle "piece_idle" fps 1\n';(OUT/f'part_{i:02d}.qc').write_text(gqc);gibfiles.append(f'part_{i:02d}')
    (OUT/'abdomen.smd').write_text(mesh_smd(models[d['attachments'][0]['model_id']]))
    qc+='$body abdomen "abdomen"\n'
    # Piece bounds and bind matrix are compiled in exactly the same coordinate basis as actor bones.
    (OUT/'piece_idle.smd').write_text('version 1\nnodes\n0 "piece" -1\nend\nskeleton\ntime 0\n0 0 0 0 0 0 0\nend\n')
    seq_report=[]
    for name,mid,activity,loop,speed,event in SEQS:
        pos,rot=frames(mid)
        pos*=scale;pos[:,0,2]+=floor
        # Remove root travel before anchoring the visible lower body.
        root_delta=pos[-1,0,:2]-pos[0,0,:2];pos[:,0,:2]=pos[0,0,:2]
        if name.startswith('prone_attack_'):
            # Keep the full body in Steve's grounded idle pose, then retarget
            # the original, clearly readable standing slash onto both arm
            # chains. Apply the source's rotational change over the prone bind
            # pose so the hands sweep forward instead of trembling in place.
            base_pos,base_rot=frames(0x3E7)
            pose_pos=base_pos[0].copy();pose_pos*=scale;pose_pos[0,2]+=floor
            pose_rot=base_rot[0].copy()
            arm_pos,arm_rot=pos.copy(),rot.copy()
            pos=np.repeat(pose_pos[None,:,:],len(arm_rot),axis=0)
            rot=np.repeat(pose_rot[None,:,:],len(arm_rot),axis=0)
            active=(3,4,5) if name=='prone_attack_right' else ((6,7,8) if name=='prone_attack_left' else (3,4,5,6,7,8))
            for node in active:
                start=Rotation.from_matrix(arm_rot[0,node])
                for f in range(len(arm_rot)):
                    delta=start.inv()*Rotation.from_matrix(arm_rot[f,node])
                    rot[f,node]=(Rotation.from_matrix(rot[f,node])*Rotation.from_rotvec(delta.as_rotvec()*0.70)).as_matrix()
        if name.startswith('prone_') or name.startswith('crawl'):
            # hzom's original actor hides the legs. Preserve its complete upper
            # body motion, but use zom's grounded resting lower body for
            # Steve's visible legs instead of animating the hidden source nodes.
            leg_pos,leg_rot=frames(0x3E7)
            pos[:,9:]=leg_pos[0,9:]*scale
            rot[:,9:]=leg_rot[0,9:]
            assert parents[9]==0
            # The authored root pitches during a strike/death. Countertransform
            # the pelvis so its resting world pose stays on the floor rather
            # than rotating the visible legs through it with the upper body.
            resting_p=leg_pos[0,0]*scale+np.array([0,0,floor])+leg_rot[0,0]@(leg_pos[0,9]*scale)
            resting_r=leg_rot[0,0]@leg_rot[0,9]
            for f in range(len(pos)):
                pos[f,9]=rot[f,0].T@(resting_p-pos[f,0])
                rot[f,9]=rot[f,0].T@resting_r
        if name=='crawl_idle':pos=pos[:1];rot=rot[:1]
        # Entity navigation owns translation. Retain authored vertical motion but remove root travel.
        # Export root motion only into QC LX extraction so StudioMDL computes native ground speed.
        if speed:pos[:,0,0]=pos[0,0,0]+np.linspace(0,speed*(len(pos)-1)/30,len(pos))
        # StudioMDL forces a loop's final pose to its first. Make that explicit
        # in the SMD so readback and original-to-GoldSrc closure are auditable.
        if loop and len(pos)>1:
            pos[-1,1:]=pos[0,1:];pos[-1,0,1:]=pos[0,0,1:];rot[-1]=rot[0]
        with warnings.catch_warnings():
            warnings.simplefilter('ignore');angles=Rotation.from_matrix(rot.reshape(-1,3,3)).as_euler('xyz').reshape(*rot.shape[:2],3)
        smd=node_text+'skeleton\n'
        for f in range(len(pos)):
            smd+=f'time {f}\n'+''.join(f'{i} '+' '.join(f'{x:.7f}' for x in (*pos[f,i],*angles[f,i]))+'\n' for i in range(16))
        (OUT/(name+'.smd')).write_text(smd+'end\n')
        # Original Steve strike records at 0x00592F18[8][0]: ticks 41/54/40.
        # Legacy disabled prone clips retain their original upright source ticks 50/55,
        # crawler tick 3 (60 Hz), rounded to stored frame 2 (30 Hz).
        event_frame={0x3EA:21,0x3E9:27,0x3C9:20,0x3F4:25,0x3F5:28}.get(mid,0) if event else 0
        if event:assert 0<event_frame<len(pos)
        qc+=f'$sequence {name} "{name}" fps 30'+(' loop' if loop else '')+(' LX' if speed else '')
        if event:qc+=' { event '+str(event)+' '+str(event_frame)+' }'
        qc+='\n';seq_report.append(dict(name=name,source_id=mid,frames=len(pos),activity=activity,loop=loop,speed=speed,event=event,event_frame=event_frame,discarded_root_travel=root_delta.tolist()))
    (OUT/'steve.qc').write_text(qc)
    for stem in ['steve',*gibfiles]:
        r=subprocess.run([str(COMPILER),str(OUT/(stem+'.qc'))],capture_output=True,text=True)
        (OUT/(stem+'_compile.log')).write_text(r.stdout+r.stderr)
        if r.returncode or not (OUT/(stem+'.mdl')).exists():raise RuntimeError(r.stdout+r.stderr)
    data=bytearray((OUT/'steve.mdl').read_bytes());assert data[:4]==b'IDST'
    seqoff=struct.unpack_from('<i',data,168)[0]
    for i,seq in enumerate(seq_report):struct.pack_into('<ii',data,seqoff+i*176+40,seq['activity'],1)
    nt,to=struct.unpack_from('<ii',data,180)
    for i in range(nt):
        at=to+i*80
        if data[at:at+1]==b'{':struct.pack_into('<i',data,at+64,struct.unpack_from('<i',data,at+64)[0]|64)
    (OUT/'steve.mdl').write_bytes(data)
    # Gib models use the same masked textures and this old compiler likewise
    # needs explicit STUDIO_NF_MASKED on the final texture records.
    for stem in gibfiles:
        path=OUT/(stem+'.mdl');g=bytearray(path.read_bytes());gn,gt=struct.unpack_from('<ii',g,180)
        for i in range(gn):
            at=gt+i*80
            if g[at:at+1]==b'{':struct.pack_into('<i',g,at+64,struct.unpack_from('<i',g,at+64)[0]|64)
        path.write_bytes(g)
    charred=bytearray(data);nt,to=struct.unpack_from('<ii',data,180)
    for i in range(nt):
        w,h,pixels=struct.unpack_from('<iii',data,to+i*80+68);pal=pixels+w*h
        charred[pal:pal+768]=bytes(round(c*.73) for c in data[pal:pal+768])
    (OUT/'steve_charred.mdl').write_bytes(charred)
    # StudioMDL rotates source 90 degrees with the compensating QC -90: verify the compiled bones.
    nb,bo=struct.unpack_from('<ii',data,140);assert nb==16
    compiled={}
    for i in range(nb):
        at=bo+i*112;name=bytes(data[at:at+32]).split(b'\0')[0].decode();compiled[name]=i
    assert set(compiled)==set(NAMES)
    report=dict(source='HOTD2 PC original znsam2b.bin/zom.bin/hzom.bin',scale=scale,floor=floor,bones=compiled,bodygroups=bodygroups,hitboxes=hitboxes,sequences=seq_report,textures=len(textures),main_triangles=810,sha256={},
        animation_mapping_evidence={'kind':8,'hit_motion_table':'0x00592FC8[kind][variant][0x004C84A8[slot]]',
        'head':0x3D1,'chest':0x3D6,'stomach':0x3CE,'right_arm':0x3D5,'left_arm':0x3D3,'right_leg':0x3C1,'left_leg':0x3C0,
        'attack_table':'0x00592F18[8][0] shared 16-node strike bank','attack_hit_ticks_60Hz':{'0x3EA':41,'0x3E9':54,'0x3C9':40,'0x3F4':50,'0x3F5':55}})
    DEST.mkdir(parents=True,exist_ok=True)
    backup=MOD/'backups/before-hotd2-steve';backup.mkdir(parents=True,exist_ok=True)
    for path in [OUT/'steve.mdl',OUT/'steve_charred.mdl',*[OUT/(stem+'.mdl') for stem in gibfiles]]:
        dst=DEST/path.name
        if dst.exists() and not (backup/path.name).exists():shutil.copy2(dst,backup/path.name)
        shutil.copy2(path,dst);report['sha256'][dst.relative_to(MOD).as_posix()]=hashlib.sha256(path.read_bytes()).hexdigest()
    (BASE/'conversion-report.json').write_text(json.dumps(report,indent=2)+'\n')
    # Constants come from the compiled file, rather than assuming bone/body ordering.
    header='// Generated by tools/convert_hotd2_steve.py.\n#pragma once\nnamespace steve_assets {\n'
    header+='static constexpr int bone[16]={'+','.join(str(compiled[n]) for n in NAMES)+'};\n'
    seq_index={entry[0]:i for i,entry in enumerate(SEQS)}
    for name,constant in [('walk','seqWalk'),('run','seqRun'),('attack_right','seqAttackRight'),('attack_left','seqAttackLeft'),('attack_both','seqAttackBoth'),('crawl','seqCrawl'),('crawl_idle','seqCrawlIdle'),('prone_attack_right','seqProneAttackRight'),('prone_attack_left','seqProneAttackLeft'),('prone_attack_both','seqProneAttackBoth'),('prone_die','seqProneDie')]:
        header+=f'static constexpr int {constant}={seq_index[name]};\n'
    header+='static constexpr int parent[16]={'+','.join(map(str,parents))+'};\n'
    header+='static constexpr int bodyBase[16]={0,'+','.join(map(str,bases))+'};\n'
    header+='static constexpr int bodyCount[16]={1,'+','.join(map(str,counts))+'};\n'
    header+='static constexpr int hitgroup[16]={'+','.join(map(str,GROUP))+'};\n'
    header+='static constexpr float hitbox[16][6]={{0,0,0,0,0,0},'+','.join('{'+','.join(f'{x:.6f}f' for x in (*h['minimum'],*h['maximum']))+'}' for h in hitboxes)+'};\n'
    header+='static constexpr float partBounds[16][4][6]={'+','.join('{'+','.join('{'+','.join(f'{x:.6f}f' for x in box)+'}' for box in part)+'}' for part in part_bounds)+'};\n'
    header+='static constexpr float duration['+str(len(seq_report))+']={'+','.join(f'{max(1,s["frames"]-1)/30:.7f}f' for s in seq_report)+'};\n'
    header+='}\n';(MOD/'source/steve_assets.h').write_text(header)
    print(f'Steve compiled/installed: {nb} bones, {len(textures)} textures, {len(seq_report)} sequences, {len(gibfiles)} regional gib models')

if __name__=='__main__':main()
