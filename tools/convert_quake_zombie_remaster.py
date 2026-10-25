"""Convert the installed Quake rerelease MD5 zombie to bounded GoldSrc skinning.

GoldSrc has rigid single-bone vertices. Fit up to 120 rigid skin patches to the
actual weighted MD5 animation, preserving all poses with measured error. Keep a
separate real hand joint for the server projectile attachment.
"""
from pathlib import Path
import json, re, struct, subprocess, warnings
import numpy as np
from scipy.spatial.transform import Rotation
from PIL import Image

MOD = Path(__file__).resolve().parents[1]
BASE = MOD / 'assets-source/quake-zombie-remaster'
OUT = BASE / 'goldsrc'
SCALE = 2.0
PAK = MOD.parent.parent / 'Quake/rerelease/id1/pak0.pak'
COMPILER = MOD.parent.parent / 'Half-Life SDK/Model Tools/studiomdl.exe'

def pak_read(path, wanted):
    result = {}
    with path.open('rb') as f:
        magic, off, size = struct.unpack('<4sii', f.read(12))
        assert magic == b'PACK'
        f.seek(off)
        records = list(struct.iter_unpack('<56sii', f.read(size)))
        for name, start, length in records:
            name = name.split(b'\0')[0].decode()
            if name in wanted:
                f.seek(start); result[name] = f.read(length)
    assert set(result) == set(wanted), set(wanted) - set(result)
    return result

def block(text, label):
    return re.search(r'\b' + re.escape(label) + r'\s*\{(.*?)\}', text, re.S)[1]

def quaternion(xyz):
    xyz = np.asarray(xyz)
    return np.concatenate([xyz, -np.sqrt(np.maximum(0, 1-(xyz*xyz).sum(axis=-1, keepdims=True)))], axis=-1)

def md5_data():
    resources = pak_read(PAK, ['progs/zombie.md5mesh', 'progs/zombie.md5anim', 'progs/zombie_00_00.lmp', 'progs/zombie.mdl', 'gfx/palette.lmp'])
    BASE.mkdir(parents=True, exist_ok=True); OUT.mkdir(exist_ok=True)
    for name, data in resources.items(): (BASE / Path(name).name).write_bytes(data)
    mesh = resources['progs/zombie.md5mesh'].decode()
    anim = resources['progs/zombie.md5anim'].decode()
    pattern = r'"([^"]+)"\s+(-?\d+)\s*\(\s*([^)]*)\)\s*\(\s*([^)]*)\)'
    joints = [(n, int(p), np.fromstring(pos, sep=' '), np.fromstring(q, sep=' '))
              for n,p,pos,q in re.findall(pattern, block(mesh, 'joints'))]
    nj = len(joints)
    bind_r = Rotation.from_quat(quaternion(np.array([j[3] for j in joints]))).as_matrix()
    bind_p = np.array([j[2] for j in joints])
    verts = re.findall(r'\bvert\s+\d+\s*\(\s*([^)]*)\)\s+(\d+)\s+(\d+)', mesh)
    triangles = np.array([tuple(map(int,t)) for t in re.findall(r'\btri\s+\d+\s+(\d+)\s+(\d+)\s+(\d+)', mesh)])
    weights = [(int(j),float(w),np.fromstring(p,sep=' ')) for j,w,p in re.findall(r'\bweight\s+\d+\s+(\d+)\s+([\d.eE+-]+)\s*\(\s*([^)]*)\)', mesh)]
    uv = np.array([np.fromstring(v[0], sep=' ') for v in verts])
    wm = np.zeros((len(verts), nj))
    owner=[]; joint_ids=[]; bias=[]; local=[]
    for i,(_,start,count) in enumerate(verts):
        for j,w,p in weights[int(start):int(start)+int(count)]:
            wm[i,j]+=w; owner.append(i); joint_ids.append(j); bias.append(w); local.append(p)
    owner=np.array(owner);joint_ids=np.array(joint_ids);bias=np.array(bias);local=np.array(local)
    hierarchy = [(n,int(p),int(flags),int(start)) for n,p,flags,start in re.findall(r'"([^"]+)"\s+(-?\d+)\s+(\d+)\s+(\d+)',block(anim,'hierarchy'))]
    assert [x[0] for x in hierarchy] == [x[0] for x in joints]
    base = np.array([np.concatenate([np.fromstring(p,sep=' '),np.fromstring(q,sep=' ')])
                     for p,q in re.findall(r'\(\s*([^)]*)\)\s*\(\s*([^)]*)\)',block(anim,'baseframe'))])
    rawframes = re.findall(r'\bframe\s+\d+\s*\{([^}]*)\}',anim,re.S)
    matrices=[];positions=[]
    for raw in rawframes:
        values=np.fromstring(raw,sep=' ');pose=base.copy()
        for i,(_,parent,flags,start) in enumerate(hierarchy):
            used=[k for k in range(6) if flags&(1<<k)];pose[i,used]=values[start:start+len(used)]
        rotations=Rotation.from_quat(quaternion(pose[:,3:])).as_matrix();p=pose[:,:3].copy()
        for i,(_,parent,_,_) in enumerate(hierarchy):
            if parent>=0:p[i]=p[parent]+rotations[parent]@p[i];rotations[i]=rotations[parent]@rotations[i]
        matrices.append(rotations);positions.append(p)
    matrices=np.array(matrices);positions=np.array(positions)
    bind=np.zeros((len(verts),3));posed=np.zeros((len(rawframes),len(verts),3))
    for w in range(len(owner)):
        i,j=owner[w],joint_ids[w]
        bind[i]+=bias[w]*(bind_r[j]@local[w]+bind_p[j])
        posed[:,i]+=bias[w]*(np.einsum('fij,j->fi',matrices[:,j],local[w])+positions[:,j])
    # All original Quake ground frames are centered 24 units above the feet.
    # Raise the global rig with its complete animation, then scale uniformly.
    floor = -float(posed[0,:,2].min())
    bind[:,2]+=floor;posed[:,:,2]+=floor;positions[:,:,2]+=floor
    bind*=SCALE;posed*=SCALE;positions*=SCALE
    return resources,joints,bind,posed,wm,uv,triangles,matrices,positions,floor

def fit_skin(bind, posed, weights, count=106):
    # Similar skin weights imply similar transforms. Position term splits wide
    # blended areas into small pieces without breaking shared-vertex topology.
    features=np.concatenate([weights,bind*.002],axis=1)
    chosen=[int(np.argmax(np.linalg.norm(bind-bind.mean(0),axis=1)))];dist=np.full(len(bind),np.inf)
    for _ in range(1,min(count,len(bind))):
        dist=np.minimum(dist,((features-features[chosen[-1]])**2).sum(1));chosen.append(int(dist.argmax()))
    centers=features[chosen].copy()
    for _ in range(25):
        labels=((features*features).sum(1)[:,None]+(centers*centers).sum(1)[None,:]-2*features@centers.T).argmin(1)
        for k in range(len(centers)):
            if np.any(labels==k):centers[k]=features[labels==k].mean(0)
    active=np.unique(labels);labels=np.searchsorted(active,labels);count=len(active)
    translations=np.zeros((len(posed),count,3));rotations=np.zeros((len(posed),count,3,3));pivots=np.zeros((count,3));approx=np.zeros_like(posed)
    for k in range(count):
        ids=np.where(labels==k)[0];pivots[k]=bind[ids].mean(0);source=bind[ids]-pivots[k]
        for f in range(len(posed)):
            target=posed[f,ids];center=target.mean(0)
            if len(ids)>=2:
                u,_,vt=np.linalg.svd(source.T@(target-center));r=vt.T@u.T
                if np.linalg.det(r)<0:vt[-1]*=-1;r=vt.T@u.T
            else:r=np.eye(3)
            translations[f,k]=center;rotations[f,k]=r;approx[f,ids]=source@r.T+center
    error=np.linalg.norm(approx-posed,axis=2)
    return labels,pivots,translations,rotations,dict(mean=float(error.mean()),p99=float(np.quantile(error,.99)),maximum=float(error.max()))

def main():
    resources,joints,bind,posed,weights,uv,triangles,joint_r,joint_p,floor=md5_data()
    labels,pivots,positions,rotations,error=fit_skin(bind,posed,weights)
    print('Rigid-skin approximation in installed units:',error,flush=True)
    assert error['p99']<2 and error['mean']<.5, error
    # Keep the real MD5 skeleton for anatomical hitboxes and attachments. Skin
    # patches are children of their dominant source joint, with relative tracks.
    nj=len(joints);skin_parents=np.array([int(weights[labels==k].mean(0).argmax()) for k in range(len(pivots))])
    global_p=np.concatenate([joint_p,positions],axis=1)
    global_r=np.concatenate([joint_r,rotations],axis=1)
    bind_jp=np.array([j[2] for j in joints]);bind_jp[:,2]+=floor;bind_jp*=SCALE
    bind_jr=Rotation.from_quat(quaternion(np.array([j[3] for j in joints]))).as_matrix()
    ref_p=np.concatenate([bind_jp,pivots],axis=0)
    ref_r=np.concatenate([bind_jr,np.repeat(np.eye(3)[None],len(pivots),axis=0)],axis=0)
    parents=[j[1] for j in joints]+skin_parents.tolist()
    names=[j[0] for j in joints]+[f'skin_{i}' for i in range(len(pivots))]
    assert len(names)<=128
    positions=global_p.copy();rotations=global_r.copy()
    local_ref_p=ref_p.copy();local_ref_r=ref_r.copy()
    for i,parent in enumerate(parents):
        if parent>=0:
            inverse=global_r[:,parent].transpose(0,2,1)
            positions[:,i]=np.einsum('fij,fj->fi',inverse,global_p[:,i]-global_p[:,parent])
            rotations[:,i]=inverse@global_r[:,i]
            local_ref_p[i]=ref_r[parent].T@(ref_p[i]-ref_p[parent])
            local_ref_r[i]=ref_r[parent].T@ref_r[i]
    labels+=nj
    nodes='version 1\nnodes\n'+''.join(f'{i} "{n}" {parents[i]}\n' for i,n in enumerate(names))+'end\n'
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        bind_angles=Rotation.from_matrix(local_ref_r).as_euler('xyz')
    reference=nodes+'skeleton\ntime 0\n'+''.join(str(i)+' '+' '.join(f'{x:.7f}' for x in (*p,*bind_angles[i]))+'\n' for i,p in enumerate(local_ref_p))+'end\ntriangles\n'
    # MD5 and StudioMDL use opposite winding. GoldSrc culls the reversed side;
    # Blender's default double-sided material concealed this in the first QA.
    triangles=triangles[:,::-1]
    # Vertex normals average all neighboring triangles, preserving the HD mesh's
    # smooth surface instead of a separate face normal at every polygon.
    normals=np.zeros_like(bind)
    for tri in triangles:
        a,b,c=bind[tri];n=np.cross(b-a,c-a)
        for i in tri:normals[i]+=n
    normals/=np.maximum(1e-10,np.linalg.norm(normals,axis=1,keepdims=True))
    for tri in triangles:
        reference+='zombie_remaster.bmp\n'
        for i in tri:
            # MD5 UVs are top-down; SMD V is bottom-up.
            row=(*bind[i],*normals[i],uv[i,0],1-uv[i,1])
            reference+=str(labels[i])+' '+' '.join(f'{x:.7f}' for x in row)+'\n'
    (OUT/'zombie_ref.smd').write_text(reference+'end\n')
    lmp=resources['progs/zombie_00_00.lmp'];w,h=struct.unpack_from('<ii',lmp)
    im=Image.frombytes('P',(w,h),lmp[8:8+w*h]);im.putpalette(resources['gfx/palette.lmp']);im.save(OUT/'zombie_remaster.bmp')
    # The MD5 poses correspond one-for-one to the original model's named frames.
    # Preserve SGClassic sequence indices so saved state and attack logic agree.
    sequences=[('idle',0,15,True,1),('walk',15,19,True,3),('run',34,18,True,4),
        ('attack',52,13,False,28),('pain',91,12,False,26),('death',192,6,False,36),
        ('painb',103,28,False,0),('painc',131,18,False,0),('paind',149,13,False,0),
        ('paine',162,30,False,27),('attb',65,14,False,28),('attc',79,12,False,28),('die_fall',162,11,False,36)]
    qc=f'$modelname "{(OUT/"zombie.mdl").as_posix()}"\n$cd "{OUT.as_posix()}"\n$cdtexture "{OUT.as_posix()}"\n$origin 0 0 0 -90\n$body zombie "zombie_ref"\n$attachment 0 "hand_R" 0 0 0\n$eyeposition 0 0 {40*SCALE}\n'
    with warnings.catch_warnings():
        warnings.simplefilter('ignore')
        eulers=Rotation.from_matrix(rotations.reshape(-1,3,3)).as_euler('xyz').reshape(*rotations.shape[:2],3)
    sequence_report=[]
    for label,start,count,loop,activity in sequences:
        frames=list(range(start,start+count))
        if loop:frames.append(start)
        smd=nodes+'skeleton\n'
        for t,f in enumerate(frames):
            smd+=f'time {t}\n'
            for i in range(len(names)):
                smd+=str(i)+' '+' '.join(f'{x:.7f}' for x in (*positions[f,i],*eulers[f,i]))+'\n'
        (OUT/f'{label}.smd').write_text(smd+'end\n')
        qc+=f'$sequence {label} "{label}" fps 10'+(' loop' if loop else '')+'\n'
        sequence_report.append(dict(name=label,start=start,source_frames=count,frames=len(frames),activity=activity))
    # Explicit hitboxes follow the original anatomical joints, never the skin
    # approximation patches. Bounds are recomputed in each joint's bind space.
    hitboxes=[]
    regions={'hip':3,'chest':2,'face':1,'upper_arm_R':5,'lower_arm_R':5,'hand_R':5,
        'upper_arm_L':4,'lower_arm_L':4,'hand_L':4,'upper_leg_R':7,'lower_leg_R':7,
        'foot_R':7,'upper_leg_L':6,'lower_leg_L':6,'foot_L':6}
    for name,group in regions.items():
        j=next(i for i,bone in enumerate(joints) if bone[0]==name)
        ids=np.where(weights[:,j]>.35)[0];assert len(ids)>0,name
        points=(bind[ids]-bind_jp[j])@bind_jr[j]
        low=points.min(0)-.5;high=points.max(0)+.5
        qc+=f'$hbox {group} "{name}" '+' '.join(f'{x:.6f}' for x in (*low,*high))+'\n'
        hitboxes.append(dict(bone=name,group=group,minimum=low.tolist(),maximum=high.tolist()))
    qc_path=OUT/'zombie.qc';qc_path.write_text(qc)
    result=subprocess.run([str(COMPILER),str(qc_path)],capture_output=True,text=True)
    (OUT/'compile.log').write_text(result.stdout+result.stderr)
    if result.returncode:raise RuntimeError(result.stdout+result.stderr)
    data=bytearray((OUT/'zombie.mdl').read_bytes())
    # Explicit activities for server tooling; no native animation events are used.
    off=struct.unpack_from('<i',data,168)[0]
    for i,seq in enumerate(sequence_report):struct.pack_into('<ii',data,off+i*176+40,seq['activity'],1 if seq['activity'] else 0)
    (OUT/'zombie.mdl').write_bytes(data)
    report=dict(source=str(PAK),model='progs/zombie.md5mesh',source_joints=len(joints),source_frames=len(posed),
        vertices=len(bind),triangles=len(triangles),goldsrc_bones=len(names),scale=SCALE,ground_offset_source_units=floor,
        idle_bounds=[posed[0].min(0).tolist(),posed[0].max(0).tolist()],skin_approximation_units=error,
        sequences=sequence_report,texture=[w,h],attachment='hand_R',hitboxes=hitboxes)
    (BASE/'conversion-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
