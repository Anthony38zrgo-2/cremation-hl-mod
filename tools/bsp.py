"""Read-only GoldSrc BSP geometry queries and entity-lump replacement."""
import struct, re, math
from pathlib import Path

class BSP:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        assert struct.unpack_from('<i', self.data)[0] == 30
        self.lumps = [struct.unpack_from('<ii', self.data, 4+i*8) for i in range(15)]
        self.planes = self.records(1, '<4fi')
        self.nodes = self.records(5, '<i2h6h2H')
        self.clips = self.records(9, '<i2h')
        self.leaves = self.records(10, '<ii6h2H4B')
        self.models = self.records(14, '<9f7i')
        self.entities = [dict(re.findall(r'"([^"]*)"\s*"([^"]*)"', s)) for s in re.findall(r'\{([^{}]*)\}',self.lump(0).decode('latin1'))]
        self.solids = [0] + [int(e['model'][1:]) for e in self.entities if e.get('classname') in ('func_wall','func_breakable','func_door','func_rotating') and e.get('model','').startswith('*')]

    def lump(self, n):
        o,s=self.lumps[n]
        return self.data[o:o+s]

    def records(self,n,fmt):
        return list(struct.iter_unpack(fmt,self.lump(n)))

    def contents(self,p,hull=0,model=0):
        node=self.models[model][9+hull]
        while node>=0:
            rec=(self.nodes if hull==0 else self.clips)[node]
            plane=self.planes[rec[0]]
            d=sum(p[i]*plane[i] for i in range(3))-plane[3]
            node=rec[1 if d>=0 else 2]
        return self.leaves[-node-1][0] if hull==0 else node

    def clear(self,p,hull=1):
        return all(self.contents(p,hull,m)!=-2 for m in self.solids)

    def floor(self,x,y,top=64,bottom=-1000):
        # Hull 1: a standing human hull, centered 36 units above its feet.
        last=None
        for z in range(top,bottom,-4):
            if self.clear((x,y,z),1):
                last=z
            elif last is not None:
                lo,hi=z,last
                for _ in range(10):
                    mid=(lo+hi)/2
                    if self.clear((x,y,mid),1): hi=mid
                    else: lo=mid
                feet=hi-36
                if self.contents((x,y,feet+2))!=-1: return None
                if self.clear((x,y,feet-2),0): return None
                return round(feet,2)
        return None

    def line_clear(self,a,b):
        length=math.dist(a,b)
        steps=max(1,math.ceil(length/4))
        return all(self.clear(tuple(a[j]+(b[j]-a[j])*i/steps for j in range(3)),0) for i in range(steps+1))

    def write(self,path,entities):
        raw=('\n'.join('{\n'+'\n'.join('"'+k+'" "'+str(v)+'"' for k,v in e.items())+'\n}' for e in entities)+'\n\0').encode('latin1')
        data=bytearray(self.data)
        while len(data)%4: data.append(0)
        struct.pack_into('<ii',data,4,len(data),len(raw))
        data.extend(raw)
        Path(path).write_bytes(data)
