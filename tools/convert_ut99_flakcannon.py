"""Convert extracted UT Flak Cannon vertex poses to a bounded GoldSrc bone rig."""
from pathlib import Path
import struct,subprocess,shutil,json
import numpy as np
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1];BASE=ROOT/'assets-source/ut99_flakcannon';OUT=BASE/'goldsrc';OUT.mkdir(exist_ok=True)
textures=['Flak_t1','Flak_t2','Flak_t3','Flak_t4','Flak_t']
for tex in textures:Image.open(BASE/'export/Botpack/Texture'/(tex+'.tga')).convert('RGB').quantize(colors=256).save(OUT/(tex+'.bmp'))
im=Image.new('RGB',(64,32),(8,18,8));ImageDraw.Draw(im).text((18,8),'30',fill=(255,90,25));im.quantize(colors=256).save(OUT/'flak_led.bmp')
report={}
def convert(mesh,target,mats,ref,scale,offset,sequences,bones=120):
 data=(BASE/'export/Botpack/VertMesh'/(mesh+'_d.3d')).read_bytes();raw=(BASE/'export/Botpack/VertMesh'/(mesh+'_a.3d')).read_bytes();nt,nv=struct.unpack_from('<HH',data);frames,framebytes=struct.unpack_from('<HH',raw)
 a=np.frombuffer(raw[4:],dtype='<u4').reshape(frames,nv);v=np.stack([a&2047,(a>>11)&2047,a>>22],-1).astype(float)
 for axis,bits in enumerate([11,11,10]):v[...,axis]=np.where(v[...,axis]>=2**(bits-1),v[...,axis]-2**bits,v[...,axis])
 v[...,2]*=2;v[...,1]*=-1;v*=scale;v+=offset
 delta=v-v[ref:ref+1];features=np.concatenate([delta.transpose(1,0,2).reshape(nv,-1),v[ref]*.2],axis=1);k=min(bones,nv)
 chosen=[0];dist=np.full(nv,np.inf)
 for i in range(1,k):dist=np.minimum(dist,((features-features[chosen[-1]])**2).sum(1));chosen.append(int(dist.argmax()))
 centers=features[chosen].copy()
 for iteration in range(12):
  distances=((features[:,None,:]-centers[None,:,:])**2).sum(2);labels=distances.argmin(1)
  for j in range(k):
   if (labels==j).any():centers[j]=features[labels==j].mean(0)
 # Bone offsets reproduce each group's average vertex displacement per frame.
 trajectories=np.zeros((frames,k,3));bind=np.zeros((k,3))
 for j in range(k):
  mask=labels==j
  if mask.any():bind[j]=v[ref,mask].mean(0);trajectories[:,j]=bind[j]+delta[:,mask].mean(1)
 prefix='version 1\nnodes\n'+''.join(f'{j} "pulse_{j}" -1\n' for j in range(k))+'end\nskeleton\ntime 0\n'+''.join(f'{j} '+' '.join(f'{x:.6f}' for x in bind[j])+' 0 0 0\n' for j in range(k))+'end\n'
 lines=[prefix,'triangles\n']
 for i in range(nt):
  off=48+i*16;ids=list(struct.unpack_from('<3H',data,off));uv=list(data[off+8:off+14]);mat=data[off+14];points=v[ref,ids];n=np.cross(points[1]-points[0],points[2]-points[0]);n/=max(np.linalg.norm(n),1e-9)
  lines.append(mats[min(mat,len(mats)-1)]+'.bmp\n')
  # Flipped Y requires reversed winding and corresponding UV order.
  for corner in [2,1,0]:
   vid=ids[corner];lines.append(str(labels[vid])+' '+' '.join(f'{x:.6f}' for x in (*points[corner],*(-n),uv[corner*2]/255,1-uv[corner*2+1]/255))+'\n')
 lines.append('end\n');(OUT/(mesh+'_ref.smd')).write_text(''.join(lines))
 front=v[ref,:,1].min();tip=v[ref][v[ref,:,1]<front+2].mean(0);vertex=int(np.linalg.norm(v[ref]-tip,axis=1).argmin());muzzleBone=int(labels[vertex]);muzzleOffset=tip-bind[muzzleBone]
 qc=f'$modelname "{(ROOT/"models/cremation"/target).as_posix()}"\n$cd "{OUT.as_posix()}"\n$cdtexture "{OUT.as_posix()}"\n$body pulse "{mesh}_ref"\n$attachment 0 "pulse_{muzzleBone}" {muzzleOffset[0]:.6f} {muzzleOffset[1]:.6f} {muzzleOffset[2]:.6f}\n'
 nodes=prefix.split('skeleton')[0]
 for name,start,count,fps,loop in sequences:
  text=nodes+'skeleton\n'
  for t,f in enumerate(range(start,start+count)):
   text+=f'time {t}\n'+''.join(f'{j} '+' '.join(f'{x:.6f}' for x in trajectories[min(f,frames-1),j])+' 0 0 0\n' for j in range(k))
  text+='end\n';(OUT/(mesh+'_'+name+'.smd')).write_text(text);qc+=f'$sequence {name} "{mesh}_{name}" fps {fps} '+('loop' if loop else '')+'\n'
 path=OUT/(mesh+'.qc');path.write_text(qc);subprocess.run([str(ROOT.parent.parent/'Half-Life SDK/Model Tools/studiomdl.exe'),str(path)],check=True,stdout=subprocess.DEVNULL)
 approximated=v[ref][None,:,:]+trajectories[:,labels]-bind[labels][None,:,:];error=np.linalg.norm(approximated-v,axis=2);report[mesh]={'triangles':nt,'vertices':nv,'frames':frames,'bones':k,'mean_pose_error_units':float(error.mean()),'max_pose_error_units':float(error.max())}
seqs=[('idle',45,1,30,True),('altfire',57,10,24,False),('fire',46,10,30,False),('loading',30,15,30,False),('loading_end',45,1,30,False),('loading_start',30,1,30,False),('draw',0,30,48,False),('holster',82,10,30,False),('sway',80,2,12,True),('all_source_poses',0,100,30,False)]
convert('flakm','v_bounce_shotgun.mdl',['Flak_t1','Flak_t2','Flak_t3','Flak_t4','flak_led'],45,.085,np.array([-12,-24,-13]),seqs)
convert('FlakHand','p_bounce_shotgun.mdl',['Flak_t'],0,.07,np.array([0,0,0]),[('idle',0,1,30,True),('fire',1,9,40,False)],100)
convert('Flak2Pick','w_bounce_shotgun.mdl',['Flak_t'],0,.065,np.array([0,-35,7]),[('idle',0,1,30,True)],1)
(BASE/'conversion-report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
