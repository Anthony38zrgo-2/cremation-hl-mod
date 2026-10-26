"""Decode the compiled MDL (not the .blend) to verify poses, parts and textures."""
from pathlib import Path
import json,struct,re
import numpy as np
from scipy.spatial.transform import Rotation
from PIL import Image
from convert_hotd2_david import MOD,BASE,OUT,DEST

class Studio:
    def __init__(self,path):
        self.data=Path(path).read_bytes();assert self.data[:8]==b'IDST\x0a\x00\x00\x00'
    def i(self,o):return struct.unpack_from('<i',self.data,o)[0]
    def pose(self,sequence,frame):
        b=self.data;so=self.i(168)+sequence*176;anim=self.i(so+124);nf=self.i(so+56);frame=min(frame,nf-1);rot=[];pos=[]
        for k in range(self.i(140)):
            bone=self.i(144)+k*112;v=np.array(struct.unpack_from('<6f',b,bone+64));scale=np.array(struct.unpack_from('<6f',b,bone+88));a=anim+k*12
            for channel,off in enumerate(struct.unpack_from('<6H',b,a)):
                if not off:continue
                p=a+off;f=frame
                while True:
                    valid,total=struct.unpack_from('<BB',b,p);assert total
                    if f<total:break
                    f-=total;p+=(valid+1)*2
                value=struct.unpack_from('<h',b,p+2*min(f+1,valid))[0];v[channel]+=value*scale[channel]
            r=Rotation.from_euler('xyz',v[3:]).as_matrix();p=v[:3];parent=self.i(bone+32)
            if parent>=0:p=pos[parent]+rot[parent]@p;r=rot[parent]@r
            rot.append(r);pos.append(p)
        return np.array(rot),np.array(pos)
    def geometry(self,sequence=0,frame=0,body=0):
        b=self.data;rot,pos=self.pose(sequence,frame);vertices=[];faces=[];uv=[];materials=[]
        skins=np.frombuffer(b,dtype='<i2',count=self.i(192),offset=self.i(200))
        for k in range(self.i(204)):
            bp=self.i(208)+k*76;nmodels,base,offset=struct.unpack_from('<iii',b,bp+64);index=(body//base)%nmodels;model=offset+index*112
            nv,bi,vi=struct.unpack_from('<iii',b,model+80)
            if not nv:continue
            v=np.frombuffer(b,dtype='<f4',count=nv*3,offset=vi).reshape(-1,3);bones=np.frombuffer(b,dtype='u1',count=nv,offset=bi);v=np.einsum('nij,nj->ni',rot[bones],v)+pos[bones];vo=len(vertices);vertices.extend(v.tolist())
            nm,mi=struct.unpack_from('<ii',b,model+72)
            for m in range(nm):
                mesh=mi+m*20;at=self.i(mesh+4);skin=self.i(mesh+8);ti=int(skins[skin]);texture=self.i(184)+ti*80;w,h=struct.unpack_from('<ii',b,texture+68)
                while True:
                    count=struct.unpack_from('<h',b,at)[0];at+=2
                    if not count:break
                    corners=[struct.unpack_from('<4h',b,at+i*8) for i in range(abs(count))];at+=abs(count)*8
                    for j in range(2,len(corners)):
                        ids=[0,j-1,j] if count<0 else ([j-2,j-1,j] if j%2==0 else [j-1,j-2,j])
                        faces.append([vo+corners[x][0] for x in ids]);uv.append([[corners[x][2]/w,1-corners[x][3]/h] for x in ids]);materials.append(ti)
        return dict(vertices=vertices,faces=faces,uv=uv,materials=materials)

def source_pose(path,frame):
    lines=path.read_text().splitlines();nodes={};frames={};section=None;f=0
    for line in lines:
        if line in ('nodes','skeleton'):section=line;continue
        if line=='end':section=None;continue
        if section=='nodes':parts=line.split();nodes[int(parts[0])]=int(parts[-1])
        if section=='skeleton':
            if line.startswith('time '):f=int(line.split()[1]);frames[f]=[]
            else:frames[f].append([float(v) for v in line.split()[1:]])
    frame=min(frame,max(frames));v=np.array(frames[frame]);v[0,0]=frames[0][0][0]
    r=Rotation.from_euler('xyz',v[:,3:]).as_matrix();p=v[:,:3].copy()
    for k in range(len(nodes)):
        parent=nodes[k]
        if parent>=0:p[k]=p[parent]+r[parent]@p[k];r[k]=r[parent]@r[k]
    return r,p

def main():
    mdl=Studio(DEST/'david.mdl');report=json.loads((BASE/'conversion-report.json').read_text());samples=[]
    delta=(MOD/'delta.lst').read_text();world=delta.split('entity_state_t gamedll',1)[1].split('entity_state_player_t gamedll',1)[0]
    assert re.search(r'DEFINE_DELTA\(\s*body,\s*DT_INTEGER,\s*32\s*,',world),'world bodygroups would be truncated on the client'
    assert mdl.i(140)==16 and mdl.i(156)==15 and mdl.i(164)==len(report['sequences']) and mdl.i(180)==28 and mdl.i(204)==16
    for index,seq in enumerate(report['sequences']):
        for f in sorted({0,(seq['frames']-1)//2,seq['frames']-1}):
            r,p=mdl.pose(index,f);rs,ps=source_pose(OUT/(seq['name']+'.smd'),f)
            error=float(np.linalg.norm(p-ps,axis=1).max());angle_error=float(np.max(np.abs(r-rs)));assert error<.08,(seq['name'],f,error);assert angle_error<.003,(seq['name'],f,angle_error)
            samples.append(dict(sequence=seq['name'],frame=f,max_bone_position_error=error,max_rotation_matrix_error=angle_error))
    mesh=mdl.geometry();assert len(mesh['faces'])==721
    pp=np.array(mesh['vertices'])[np.array(mesh['faces'])];volume=float(np.einsum('ij,ij->i',pp[:,0],np.cross(pp[:,1],pp[:,2])).sum()/6);assert volume<0,'GoldSrc clockwise winding required'
    height=float(np.ptp(np.array(mesh['vertices'])[:,2]));assert 71.8<height<72.2
    preview_dir=BASE/'compiled-preview';preview_dir.mkdir(exist_ok=True);textures=[]
    for i in range(mdl.i(180)):
        o=mdl.i(184)+i*80;name=mdl.data[o:o+64].split(b'\0')[0].decode();flags,w,h,off=struct.unpack_from('<4i',mdl.data,o+64);pixels=mdl.data[off:off+w*h];palette=mdl.data[off+w*h:off+w*h+768]
        image=Image.frombytes('P',(w,h),pixels);image.putpalette(palette)
        if name.startswith('{'):assert flags&64;image.info['transparency']=255
        path=preview_dir/f'tex_{i:02d}.png';image.save(path);textures.append(str(path))
    pose_entries=[]
    for label,seq,frame,body in [('Intacto',0,0,0),('Carrera',2,15,0),('Heridas y arrastre',20,25,2*16+3*64+3*256+2*262144+1048576)]:
        entry=mdl.geometry(seq,frame,body);entry.update(name=label,sequence=seq,frame=frame,body=body);pose_entries.append(entry)
    (BASE/'compiled-preview.json').write_text(json.dumps(dict(textures=textures,poses=pose_entries),separators=(',',':')))
    result=dict(compiled_bones=16,compiled_triangles=721,compiled_height=height,compiled_textures=28,pose_samples=samples,clockwise_signed_volume=volume)
    (BASE/'compiled-readback.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'PASS: {len(samples)} compiled poses match source SMD, 721 triangles, 28 textures, 72-unit height, original UV and clockwise winding.')

if __name__=='__main__':main()
