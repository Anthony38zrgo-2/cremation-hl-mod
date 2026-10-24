"""Generate a green ellipsoid projectile using the existing GoldSrc model container."""
from pathlib import Path
import struct as s,math
MOD=Path(__file__).resolve().parents[1]
SOURCE=MOD/'assets-source/additional_weapons/nailgun/nail.mdl'
OUT=MOD/'assets-source/linkgun_projectile/green_oval.mdl'
def build_assets():
 original=SOURCE.read_bytes()
 def integer(o):return s.unpack_from('<i',original,o)[0]
 data=bytearray(original[:integer(188)])
 def append(payload):
  while len(data)%4:data.append(0)
  offset=len(data);data.extend(payload);return offset
 vertices=[];normals=[];uv=[];rings=8;sides=12
 for i in range(rings+1):
  theta=math.pi*i/rings
  for j in range(sides+1):
   phi=math.tau*j/sides
   x=8*math.cos(theta);y=3*math.sin(theta)*math.cos(phi);z=3*math.sin(theta)*math.sin(phi)
   vertices.append((x,y,z));n=(x/64,y/9,z/9);length=math.sqrt(sum(v*v for v in n));normals.append(tuple(v/length for v in n));uv.append((round(j/sides*31),round(i/rings*31)))
 triangles=[]
 for i in range(rings):
  for j in range(sides):
   a=i*(sides+1)+j;b=a+sides+1;c=a+1;d=b+1
   if i>0:triangles.append((a,b,c))
   if i<rings-1:triangles.append((c,b,d))
 commands=bytearray()
 for triangle in triangles:
  commands.extend(s.pack('<h',3))
  for v in triangle:commands.extend(s.pack('<4h',v,v,*uv[v]))
 commands.extend(s.pack('<h',0))
 verts=append(b''.join(s.pack('<3f',*v) for v in vertices));norms=append(b''.join(s.pack('<3f',*v) for v in normals));bones=append(bytes(len(vertices)));tris=append(commands)
 mesh=append(s.pack('<5i',len(triangles),tris,0,len(normals),0))
 body=integer(208);model=s.unpack_from('<i',original,body+72)[0]
 s.pack_into('<f',data,model+68,8.6)
 s.pack_into('<8i',data,model+72,1,mesh,len(vertices),bones,verts,len(normals),bones,norms)
 bone=integer(144);s.pack_into('<6f',data,bone+64,0,0,0,0,0,0)
 anim=append(bytes(12));seq=integer(168);s.pack_into('<i',data,seq+124,anim);s.pack_into('<i',data,seq+56,1)
 for at in [88,112,seq+96]:s.pack_into('<6f',data,at,-8,-3,-3,8,3,3)
 tex=integer(184);s.pack_into('<3i',data,tex+64,4,32,32)
 while len(data)%4:data.append(0)
 s.pack_into('<i',data,188,len(data));pixel_index=len(data)
 pixels=bytes(round(100+155*math.sin(math.pi*(y+.5)/32)) for y in range(32) for x in range(32))
 palette=bytes(channel for i in range(256) for channel in (round(35*i/255),i,round(65*i/255)))
 data.extend(pixels+palette);s.pack_into('<i',data,tex+76,pixel_index);s.pack_into('<i',data,72,len(data))
 OUT.parent.mkdir(parents=True,exist_ok=True);OUT.write_bytes(data)
if __name__=='__main__':build_assets()
