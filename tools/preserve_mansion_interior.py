"""Retain exact original light samples for unchanged interior BSP surfaces."""
import base64,gzip,hashlib,json,struct,math
from pathlib import Path
from bsp import BSP

MOD=Path(__file__).resolve().parents[1]
CACHE=MOD/'assets-source/truth-exterior/interior-lightmaps.json.gz'

def surfaces(b):
    vertices=b.records(3,'<3f');edges=b.records(12,'<2H');surfedges=[r[0] for r in b.records(13,'<i')]
    texinfo=b.records(6,'<8f2i');textures=b.lump(2)
    for i,face in enumerate(b.records(7,'<Hhihh4Bi')):
        uv=texinfo[face[4]]
        off=struct.unpack_from('<i',textures,4+4*uv[8])[0]
        name=textures[off:off+16].split(b'\0')[0].decode('latin1')
        # The original room surfaces, furniture and fixtures. Exterior material
        # families and doors are excluded; doors intentionally change to copper.
        if not (name.startswith(('fifties_','fifts','pfab_','generic','c1a1w','+a','picture','cardbox','bcrate','lab1_','silo2_')) or name in ('c1a2_flr1','c2a4_flr1','babtech_dr1c','babtech_dr4c','{ladder2')):continue
        pts=[]
        for j in range(face[2],face[2]+face[3]):
            e=surfedges[j];pts.append(vertices[edges[abs(e)][0 if e>=0 else 1]])
        if not all(-736<=p[0]<=208 and -800<=p[1]<=288 for p in pts):continue
        key=[name,sorted(tuple(round(v,3) for v in p) for p in pts),[round(v,5) for v in uv[:8]]]
        digest=hashlib.sha256(json.dumps(key,separators=(',',':')).encode()).hexdigest()
        basis=uv[:8]
        projected=[(sum(p[k]*basis[k] for k in range(3))+basis[3],sum(p[k]*basis[k+4] for k in range(3))+basis[7]) for p in pts]
        rect=[math.floor(min(p[k] for p in projected)/16) for k in range(2)]+[math.ceil(max(p[k] for p in projected)/16) for k in range(2)]
        # CSG's plane pool can weld the same authored surface onto nearby
        # axial planes (.02 units apart) after repartitioning the exterior.
        plane=b.planes[face[0]]
        gridkey=json.dumps([name,[round(v,5) for v in basis],[*[round(v,5) for v in plane[:3]],round(plane[3],1)],face[1]],separators=(',',':'))
        yield i,digest,face,gridkey,rect

def preserve(path):
    cached=json.loads(gzip.decompress(CACHE.read_bytes())) if CACHE.exists() else {}
    if cached.get('version')!=4:
        original=BSP(MOD/'backups/before-truth-exterior/cs_mansion.bsp')
        offsets=sorted({f[-1] for f in original.records(7,'<Hhihh4Bi') if f[-1]>=0}|{len(original.lump(8))})
        ends=dict(zip(offsets,offsets[1:]));rows={}
        grids={}
        for _,key,face,gridkey,rect in surfaces(original):
            if face[-1]<0:continue
            raw=original.lump(8)[face[-1]:ends[face[-1]]]
            row=dict(styles=face[5:9],data=base64.b64encode(raw).decode(),rect=rect)
            rows[key]=row;grids.setdefault(gridkey,[]).append(row)
        CACHE.parent.mkdir(parents=True,exist_ok=True)
        cached=dict(version=4,faces=rows,grids=grids)
        CACHE.write_bytes(gzip.compress(json.dumps(cached).encode(),mtime=0))
    rows=cached['faces']
    b=BSP(path);lighting=bytearray(b.lump(8));faces=bytearray(b.lump(7));count=0;missing=[]
    resampled=0
    for i,key,face,gridkey,rect in surfaces(b):
        if face[-1]<0:continue
        row=rows.get(key)
        if row is None:
            tiles=cached['grids'].get(gridkey,[])
            if not tiles:missing.append(i);continue
            # BSP clipping may repartition an unchanged plane. Recover its
            # original 16-unit sample grid rather than accepting new lighting.
            overlaps=lambda t:max(0,min(rect[2],t['rect'][2])-max(rect[0],t['rect'][0])+1)*max(0,min(rect[3],t['rect'][3])-max(rect[1],t['rect'][1])+1)
            tiles=sorted(tiles,key=overlaps,reverse=True);styles=tiles[0]['styles'];raw=bytearray()
            for style in styles:
                if style==255:break
                for v in range(rect[1],rect[3]+1):
                    for u in range(rect[0],rect[2]+1):
                        tile=next((t for t in tiles if t['rect'][0]<=u<=t['rect'][2] and t['rect'][1]<=v<=t['rect'][3]),None)
                        if tile is None:missing.append(i);raw.extend(b'\0\0\0');continue
                        if style not in tile['styles']:raw.extend(b'\0\0\0');continue
                        a,bb,c,d=tile['rect'];w=c-a+1;h=d-bb+1
                        index=(tile['styles'].index(style)*w*h+(v-bb)*w+u-a)*3
                        raw.extend(base64.b64decode(tile['data'])[index:index+3])
            row=dict(styles=styles,data=base64.b64encode(raw).decode());resampled+=1
        offset=len(lighting);lighting.extend(base64.b64decode(row['data']))
        struct.pack_into('<4Bi',faces,i*20+12,*row['styles'],offset);count+=1
    assert not missing,f'Interior surfaces changed unexpectedly: {missing[:20]}'
    data=bytearray(b.data)
    for lump,raw in ((7,faces),(8,lighting)):
        while len(data)%4:data.append(0)
        struct.pack_into('<ii',data,4+lump*8,len(data),len(raw));data.extend(raw)
    Path(path).write_bytes(data)
    print(f'PASS: {count} interior surfaces retain original light samples ({resampled} BSP partitions remapped).')

if __name__=='__main__':
    import sys
    preserve(sys.argv[1])
