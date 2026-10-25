"""Read the compiled MDL poses back, compare to MD5, and export a QA preview."""
from pathlib import Path
import json, struct
import numpy as np
from scipy.spatial import cKDTree
from scipy.spatial.transform import Rotation
from PIL import Image
from convert_quake_zombie_remaster import MOD, BASE, OUT, md5_data

def studio_geometry(path, sequence=0, frame=0):
    data=Path(path).read_bytes()
    def integer(o):return struct.unpack_from('<i',data,o)[0]
    nb,bo=integer(140),integer(144);so=integer(168)+sequence*176
    anim=integer(so+124)
    matrices=[];positions=[]
    for b in range(nb):
        bone=bo+b*112;v=np.array(struct.unpack_from('<6f',data,bone+64));scale=np.array(struct.unpack_from('<6f',data,bone+88));a=anim+b*12
        for ch,off in enumerate(struct.unpack_from('<6H',data,a)):
            if not off:continue
            p=a+off;k=frame
            while True:
                valid,total=struct.unpack_from('<BB',data,p);assert total>0
                if k<total:break
                k-=total;p+=(valid+1)*2
            value=struct.unpack_from('<h',data,p+2*(min(k+1,valid)))[0]
            v[ch]+=value*scale[ch]
        r=Rotation.from_euler('xyz',v[3:]).as_matrix();pos=v[:3];parent=integer(bone+32)
        if parent>=0:pos=positions[parent]+matrices[parent]@pos;r=matrices[parent]@r
        matrices.append(r);positions.append(pos)
    matrices=np.array(matrices);positions=np.array(positions)
    body=integer(208);model=integer(body+72)
    nv,bi,vi=struct.unpack_from('<iii',data,model+80)
    vertices=np.frombuffer(data,dtype='<f4',count=nv*3,offset=vi).reshape(-1,3).astype(float)
    bones=np.frombuffer(data,dtype='u1',count=nv,offset=bi)
    vertices=np.einsum('nij,nj->ni',matrices[bones],vertices)+positions[bones]
    nm,mi=struct.unpack_from('<ii',data,model+72)
    faces=[];uv=[];texture_index=integer(184);tw,th,tp=struct.unpack_from('<iii',data,texture_index+68)
    for m in range(nm):
        at=integer(mi+m*20+4)
        while True:
            count=struct.unpack_from('<h',data,at)[0];at+=2
            if not count:break
            corners=[struct.unpack_from('<4h',data,at+i*8) for i in range(abs(count))];at+=8*abs(count)
            for i in range(2,len(corners)):
                ids=[0,i-1,i] if count<0 else ([i-2,i-1,i] if i%2==0 else [i-1,i-2,i])
                faces.append([corners[k][0] for k in ids]);uv.append([[corners[k][2]/tw,1-corners[k][3]/th] for k in ids])
    image=Image.frombytes('P',(tw,th),data[tp:tp+tw*th]);image.putpalette(data[tp+tw*th:tp+tw*th+768])
    na,ai=integer(212),integer(216);attachments=[]
    for n in range(na):
        a=ai+n*88;bone=integer(a+36);local=np.array(struct.unpack_from('<3f',data,a+40));attachments.append((positions[bone]+matrices[bone]@local).tolist())
    hitboxes=[]
    for n in range(integer(156)):
        bone,group,*bounds=struct.unpack_from('<ii6f',data,integer(160)+n*32)
        hitboxes.append(dict(bone=bone,group=group,minimum=bounds[:3],maximum=bounds[3:],rotation=matrices[bone],origin=positions[bone]))
    return dict(vertices=vertices,faces=faces,uv=uv,image=image,attachments=attachments,bones=nb,hitboxes=hitboxes)

def main():
    _,_,_,posed,_,_,_,_,_,_=md5_data()
    report=json.loads((BASE/'conversion-report.json').read_text());samples=[]
    for seq in (0,1,2,3,6,9,10,11):
        clip=report['sequences'][seq]
        for f in (0,clip['source_frames']//2,clip['source_frames']-1):
            mesh=studio_geometry(OUT/'zombie.mdl',seq,f)
            distance=cKDTree(posed[clip['start']+f]).query(mesh['vertices'])[0]
            samples.append(dict(sequence=clip['name'],frame=f,mean=float(distance.mean()),maximum=float(distance.max())))
            assert distance.mean()<.5 and distance.max()<4,(clip,f,distance.mean(),distance.max())
    mesh=studio_geometry(OUT/'zombie.mdl');assert mesh['bones']<=128
    assert len(mesh['faces'])==1262 and len(mesh['attachments'])==1
    assert 102<mesh['vertices'][:,2].max()<105
    assert len(mesh['hitboxes'])==15 and {b['group'] for b in mesh['hitboxes']}==set(range(1,8))
    # GoldSrc's front-face convention matches the known working SGClassic MDL.
    points=mesh['vertices'][np.array(mesh['faces'])]
    volume=np.einsum('ij,ij->i',points[:,0],np.cross(points[:,1],points[:,2])).sum()/6
    assert volume<0,'Inside-out model: StudioMDL winding was not compensated'
    mesh['image'].convert('RGB').save(BASE/'compiled-texture.png')
    preview={k:(v.tolist() if isinstance(v,np.ndarray) else v) for k,v in mesh.items() if k not in ('image','hitboxes')}
    (BASE/'compiled-preview.json').write_text(json.dumps(preview))
    result=dict(samples=samples,compiled_triangles=len(mesh['faces']),compiled_bones=mesh['bones'],height=float(mesh['vertices'][:,2].max()),attachment=mesh['attachments'][0])
    (BASE/'compiled-readback.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: 24 compiled MDL poses match the rerelease MD5, including orientation, scale, skin and attachment.')
    print('Height:',result['height'],'triangles:',result['compiled_triangles'],'bones:',result['compiled_bones'])

if __name__=='__main__':main()
