"""Preserve Ravenholm animations while making fast zombie locomotion usable by AI."""
from pathlib import Path
import struct,json,hashlib
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
def add_walk_activity(data):
 b=bytearray(data);assert b[:4]==b'IDST'
 count,offset=struct.unpack_from('<ii',b,164)
 seq=[(i,offset+i*176) for i in range(count)]
 if any(struct.unpack_from('<i',b,o+40)[0]==3 for _,o in seq):return bytes(b)
 run=next(o for _,o in seq if struct.unpack_from('<i',b,o+40)[0]==4)
 # Ravenholm's zombie movement requests ACT_WALK even for this fast model.
 # Re-map only its activity tag; preserve all names, offsets and animations.
 # Do not append sequence tables after texture data: GoldSrc can strip that
 # tail in the dedicated-server cache, causing DispatchAnimEvents to read garbage.
 struct.pack_into('<i',b,run+40,3)
 return bytes(b)

def install():
 src=ROOT/'Half-Life2Classic_Ravenholm/models/zombie_fast.mdl'
 dst=MOD/'models/zombie_fast.mdl';dst.write_bytes(add_walk_activity(src.read_bytes()))
 (MOD/'horde-assets.json').write_text(json.dumps({'models/zombie_fast.mdl':{
  'source':'Half-Life2Classic_Ravenholm/models/zombie_fast.mdl',
  'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),
  'sha256':hashlib.sha256(dst.read_bytes()).hexdigest(),
  'transform':'map run animation activity from ACT_RUN to ACT_WALK for Ravenholm zombie AI; all geometry, offsets and animation data unchanged'}},indent=2)+'\n')
 print('Installed fast zombie ACT_WALK activity mapping for native Ravenholm locomotion.')
if __name__=='__main__':install()
