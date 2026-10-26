"""Validate installed first-folder model, CS weapon assets and wave metadata."""
from pathlib import Path
import hashlib,json,struct
from czero_animation import channels
MOD=Path(__file__).resolve().parents[1];ROOT=MOD.parent
report=json.loads((MOD/'assets-source/czero-zombie/installed.json').read_text())
source=MOD/'assets-source/czero-zombie/zombie_source_FIXED/0/zombie_source.mdl'
assert report['folder']=='0' and hashlib.sha256(source.read_bytes()).hexdigest()==report['source_sha256']
path=MOD/'models/cremation/czero/zombie_knife.mdl';d=path.read_bytes()
assert d[:4]==b'IDST' and struct.unpack_from('<i',d,4)[0]==10 and struct.unpack_from('<i',d,72)[0]==len(d)
nb,bo=struct.unpack_from('<2i',d,140);nh,ho=struct.unpack_from('<2i',d,156);ns,so=struct.unpack_from('<2i',d,164)
assert nb==34 and nh==20 and ns==111
assert {struct.unpack_from('<i',d,ho+i*32+4)[0] for i in range(nh)}==set(range(1,8))
for i in (56,57,73,74,75,76):
 blends,anim=struct.unpack_from('<2i',d,so+i*176+120);assert blends==1 and 244<anim<len(d)
original=source.read_bytes();original_seq=struct.unpack_from('<i',original,168)[0]
pose=channels(d,nb,so+75*176)
names=[d[bo+i*112:bo+i*112+32].split(b'\0')[0] for i in range(nb)]
parents=[struct.unpack_from('<i',d,bo+i*112+32)[0] for i in range(nb)];spine=names.index(b'Bip01 Spine')
for index in (3,4):
 assert struct.unpack_from('<i',d,so+index*176+124)[0]<struct.unpack_from('<i',d,184)[0]
 gait=channels(d,nb,so+index*176);before=channels(original,nb,original_seq+index*176)
 for bone in range(nb):
  parent=bone
  while parent>=0 and parent!=spine:parent=parents[parent]
  for channel in range(6):
   expected=[pose[bone][channel][0]]*len(gait[bone][channel]) if parent==spine else before[bone][channel]
   assert gait[bone][channel]==expected,(index,bone,channel)
assert struct.unpack_from('<i',d,so+76*176+36)[0]&1==0
assert abs((struct.unpack_from('<i',d,so+76*176+56)[0]-1)/struct.unpack_from('<f',d,so+76*176+32)[0]-.65)<.001
charred=(path.parent/'zombie_knife_charred.mdl').read_bytes();assert len(charred)==len(d)
assert charred[140:220]==d[140:220] and charred[ho:ho+nh*32]==d[ho:ho+nh*32]
assert (path.parent/'p_knife.mdl').read_bytes()==next((MOD/'assets-source/leviathan-knife').rglob('p_knife.mdl')).read_bytes()
placement=json.loads((MOD/'mansion-placement.json').read_text());assert all(w['czero']==1 for w in placement['waves'])
assert [sum(w.values()) for w in placement['waves']]==[41,41,81,81]
assert placement['special_enemy']['spawn']=='random_unoccupied_ct'
print('PASS: folder 0, anatomical hitboxes, HL-compatible knife poses, charred rig, exact Leviathan skin and 41/41/81/81 waves.')
