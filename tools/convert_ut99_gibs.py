"""Convert UModel UE1 .3d exports to rigid GoldSrc gib bodygroups."""
from pathlib import Path
import struct,math,shutil,subprocess,json,hashlib
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];BASE=ROOT/'assets-source/ut99_gibs';OUT=BASE/'goldsrc';OUT.mkdir(exist_ok=True)
HEAD='version 1\nnodes\n0 "root" -1\nend\nskeleton\ntime 0\n0 0 0 0 0 0 0\nend\n'
ITEMS=[('Botpack','headmalem','MMT1',12),('Botpack','malearmm','malearmT',20),('Botpack','maletorsom','maletorsoT',24),('Botpack','malefootm','malefootT',14),('Botpack','ThighUTM','ThighT',18),('Botpack','femalearmm','femalearmT',18),('Botpack','HeadFemaleM','FMT1',12),('Botpack','femaletorsom','FemaleTorsoT',22),('Botpack','femalefootm','femalefootT',13),('UnrealShare','PHeartM','Jmisc1',8),('UnrealShare','LiverM','Jparts1',10),('UnrealShare','stomachM','Jparts1',12)]
def signed(x,b):return x-(1<<b) if x&(1<<(b-1)) else x
for pkg,mesh,tex,size in ITEMS:
 folder=BASE/'export'/pkg;data=(folder/'VertMesh'/(mesh+'_d.3d')).read_bytes();anim=(folder/'VertMesh'/(mesh+'_a.3d')).read_bytes();nt,nv=struct.unpack_from('<HH',data);verts=[]
 for i in range(nv):
  v=struct.unpack_from('<I',anim,4+i*4)[0];verts.append((signed(v&2047,11),signed((v>>11)&2047,11),signed(v>>22,10)*2))
 mins=[min(v[a] for v in verts) for a in range(3)];maxs=[max(v[a] for v in verts) for a in range(3)];scale=size*(2 if pkg=="UnrealShare" else 1)/max(maxs[a]-mins[a] for a in range(3));verts=[tuple((v[a]-(mins[a]+maxs[a])*.5)*scale for a in range(3)) for v in verts]
 im=Image.open(folder/'Texture'/(tex+'.tga')).convert('RGB').quantize(colors=256);im.save(OUT/(tex+'.bmp'))
 lines=[HEAD,'triangles\n']
 for j in range(nt):
  off=48+j*16;ids=struct.unpack_from('<3H',data,off);uv=list(data[off+8:off+14]);vs=[verts[k] for k in ids];a=[vs[1][k]-vs[0][k] for k in range(3)];b=[vs[2][k]-vs[0][k] for k in range(3)];n=(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]);length=math.sqrt(sum(x*x for x in n));n=tuple(x/max(length,.00001) for x in n)
  lines.append(tex+'.bmp\n')
  for k,v in enumerate(vs):lines.append('0 '+' '.join(f'{x:.6f}' for x in (*v,*n,uv[k*2]/255,1-uv[k*2+1]/255))+'\n')
 lines.append('end\n');(OUT/(mesh+'.smd')).write_text(''.join(lines))
(OUT/'idle.smd').write_text(HEAD)
# Build paired limbs while keeping gibbing as a single whole-body event.
def remesh(source,target,mirror=False,shift=(0,0,0)):
 text=(OUT/(source+'.smd')).read_text();prefix,tail=text.split('triangles\n',1);lines=tail.splitlines();blocks=[]
 for i in range(0,len(lines)-1,4):
  block=[lines[i]]
  for line in lines[i+1:i+4]:
   v=line.split();v[1:4]=[str(float(v[a+1])+shift[a]) for a in range(3)]
   if mirror:v[2]=str(-float(v[2]));v[5]=str(-float(v[5]))
   block.append(' '.join(v))
  if mirror:block=[block[0],block[3],block[2],block[1]]
  blocks.extend(block)
 (OUT/(target+'.smd')).write_text(prefix+'triangles\n'+'\n'.join(blocks)+'\nend\n')
remesh('malearmm','arm_left');remesh('malearmm','arm_right',True)
# Join the original thigh and foot as one rigid leg fragment.
remesh('ThighUTM','leg_thigh',shift=(0,0,7))
remesh('malefootm','leg_foot',shift=(0,0,-10))
a=(OUT/'leg_thigh.smd').read_text();b=(OUT/'leg_foot.smd').read_text();(OUT/'leg_left.smd').write_text(a.rsplit('end',1)[0]+b.split('triangles\n',1)[1])
remesh('leg_left','leg_right',True)
# Native CGib body indices: six human variants, four alien variants.
sets={'hgibs':['headmalem','maletorsom','arm_left','arm_right','leg_left','leg_right'],'agibs':['PHeartM','LiverM','stomachM','PHeartM']}
for name,meshes in sets.items():
 target=ROOT/'models'/(name+'.mdl');backup=ROOT/'backups/before-ut99-gibs'/target.name;backup.parent.mkdir(parents=True,exist_ok=True)
 if not backup.exists():shutil.copy2(target if target.exists() else ROOT.parent/'valve/models'/target.name,backup)
 qc=f'$modelname "{target.as_posix()}"\n$cd "{OUT.as_posix()}"\n$cdtexture "{OUT.as_posix()}"\n$bodygroup gibs\n{{\n'+''.join(f' studio "{m}"\n' for m in meshes)+'}\n$sequence idle "idle" fps 1\n';path=OUT/(name+'.qc');path.write_text(qc)
 subprocess.run([str(ROOT.parent.parent/'Half-Life SDK/Model Tools/studiomdl.exe'),str(path)],check=True)
for name,flags in [('hgibs',36),('agibs',16)]:
 p=ROOT/'models'/(name+'.mdl');b=bytearray(p.read_bytes());struct.pack_into('<I',b,136,flags);p.write_bytes(b)
manifest=ROOT/'file-manifest.json';d=json.loads(manifest.read_text())
for name in sets:d[f'models/{name}.mdl']=hashlib.sha256((ROOT/'models'/(name+'.mdl')).read_bytes()).hexdigest()
manifest.write_text(json.dumps(d,indent=2)+'\n')
(ROOT/'ut99-gib-assets.json').write_text(json.dumps({'source':'C:/UnrealTournament/System','exporter':'UE Viewer (UModel), https://www.gildor.org/projects/umodel','extracted':[x[1] for x in ITEMS],'models':sets,'conversion':'first animation frame, centered rigid mesh, original UV and textures, size adjusted for GoldSrc','backup':'backups/before-ut99-gibs'},indent=2)+'\n')
