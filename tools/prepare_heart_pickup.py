"""Reuse UT gib heart mesh; suppress gib-only engine trails for the pickup."""
from pathlib import Path
import struct
MOD=Path(__file__).resolve().parents[1]
d=bytearray((MOD/'models/agibs.mdl').read_bytes());assert d[:4]==b'IDST'
body=struct.unpack_from('<i',d,208)[0];model=struct.unpack_from('<i',d,body+72)[0]
assert d[model:model+64].split(b'\0')[0]==b'PHeartM'
struct.pack_into('<I',d,136,0)
(MOD/'models/cremation/heart_pickup.mdl').write_bytes(d)
size=16;color=(255,45,40)
sprite=bytearray(struct.pack('<4i f 3i f i',0x50534449,2,2,1,12.,size,size,1,0.,0))
sprite.extend(struct.pack('<H',256));sprite.extend(bytes(round(c*i/255) for i in range(256) for c in color))
sprite.extend(struct.pack('<5i',0,-8,8,size,size));sprite.extend(bytes([255]*(size*size)))
(MOD/'sprites/cremation/heart_red_square.spr').write_bytes(sprite)
print('Prepared existing PHeartM gib mesh and red square sprite.')
