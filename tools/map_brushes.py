"""Valve 220 brush parsing and exact convex brush subtraction helpers."""
from dataclasses import dataclass
from functools import cached_property
from itertools import combinations
from pathlib import Path
import re
import numpy as np

@dataclass
class Face:
    normal: np.ndarray
    distance: float
    texture: str

    def flipped(self):return Face(-self.normal,-self.distance,self.texture)

    def text(self):
        n=self.normal
        axis=np.eye(3)[int(np.argmin(abs(n)))]
        u=np.cross(n,axis);u/=np.linalg.norm(u)
        v=np.cross(n,u)
        p=n*self.distance
        points=[p,p+u*128,p+v*128]
        fmt=lambda a:'( '+' '.join(f'{x:.7f}'.rstrip('0').rstrip('.') if abs(x)>1e-8 else '0' for x in a)+' )'
        tail=self.texture
        if '[' not in tail:
            if abs(n[2])>.5:tail+=' [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1'
            elif abs(n[0])>.5:tail+=' [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1'
            else:tail+=' [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1'
        return ' '.join(fmt(a) for a in points)+' '+tail

class Brush:
    def __init__(self,faces):self.faces=faces
    @cached_property
    def vertices(self):
        if len(self.faces)<4:return np.empty((0,3))
        normals=np.array([f.normal for f in self.faces]);distances=np.array([f.distance for f in self.faces])
        combos=np.array(list(combinations(range(len(self.faces)),3)))
        a=normals[combos];good=abs(np.linalg.det(a))>1e-8
        if not good.any():return np.empty((0,3))
        indices=combos[good];points=np.linalg.solve(a[good],distances[indices][...,None])[...,0]
        points=points[np.all(points@normals.T>=distances[None,:]-.002,axis=1)]
        return np.unique(np.round(points,5),axis=0)
    @property
    def bounds(self):return self.vertices.min(0),self.vertices.max(0)
    def valid(self):
        v=self.vertices
        return len(v)>=4 and np.linalg.matrix_rank(v-v[0],tol=.005)==3
    def clipped(self,face):
        distances=self.vertices@face.normal-face.distance
        if distances.min()>=-.002:return self
        if distances.max()<=.002:return None
        b=Brush(self.faces+[face]);return b if b.valid() else None
    def subtract(self,cutter):
        remaining=self;pieces=[]
        for face in cutter.faces:
            if remaining is None:break
            outside=remaining.clipped(face.flipped())
            if outside:pieces.append(outside)
            remaining=remaining.clipped(face)
        return pieces
    def compact(self):
        vertices=self.vertices
        active=[f for f in self.faces if np.count_nonzero(abs(vertices@f.normal-f.distance)<.01)>=3]
        unique=[]
        for face in active:
            if not any(np.linalg.norm(face.normal-other.normal)<1e-6 and abs(face.distance-other.distance)<.005 for other in unique):unique.append(face)
        return Brush(unique)
    def text(self):return '{\n'+'\n'.join(f.text() for f in self.compact().faces)+'\n}'

def read_map(path):
    entities=[];entity=None;brush=None
    for line in Path(path).read_text(encoding='utf-8-sig').splitlines():
        line=line.strip()
        if not line or line.startswith('//'):continue
        if line=='{':
            if entity is None:entity={'keys':{},'brushes':[]}
            else:brush=[]
        elif line=='}':
            if brush is not None:entity['brushes'].append(Brush(brush));brush=None
            else:entities.append(entity);entity=None
        elif brush is not None:
            matches=list(re.finditer(r'\(([^)]*)\)',line))
            p=[np.array(list(map(float,m.group(1).split()))) for m in matches[:3]]
            n=np.cross(p[1]-p[0],p[2]-p[0]);n/=np.linalg.norm(n)
            brush.append(Face(n,float(n@p[0]),line[matches[2].end():].strip()))
        else:
            k,v=re.findall(r'"([^"]*)"',line)[:2];entity['keys'][k]=v
    return entities

def write_map(path,entities):
    text=[]
    for entity in entities:
        text.append('{\n'+'\n'.join(f'"{k}" "{v}"' for k,v in entity['keys'].items()))
        text.extend(b.text() for b in entity['brushes'])
        text.append('}')
    Path(path).write_text('\n'.join(text)+'\n',encoding='ascii')

def texture(name):return name

def box(lo,hi,tex='crete3_wall01b'):
    return Brush([Face(np.eye(3)[i]*sign,float(lo[i] if sign>0 else -hi[i]),texture(tex)) for i in range(3) for sign in (1,-1)])

def prism(poly,z0,z1,tex='crete3_wall01b'):
    poly=np.array(poly,dtype=float)
    if sum(np.cross(poly[(i+1)%len(poly)]-poly[i],poly[(i+2)%len(poly)]-poly[(i+1)%len(poly)]) for i in range(len(poly)))<0:poly=poly[::-1]
    faces=[Face(np.array([0,0,1.]),z0,texture(tex)),Face(np.array([0,0,-1.]),-z1,texture(tex))]
    for a,b in zip(poly,np.roll(poly,-1,axis=0)):
        direction=b-a;n=np.array([-direction[1],direction[0],0.]);n/=np.linalg.norm(n)
        faces.append(Face(n,float(n[:2]@a),texture(tex)))
    return Brush(faces)
