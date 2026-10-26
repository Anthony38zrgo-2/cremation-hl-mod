"""Prepare first-folder CS zombie and installed Condition Zero Leviathan knife resources."""
from pathlib import Path
import hashlib,json,struct,shutil
from czero_animation import bake_gaits
MOD=Path(__file__).resolve().parents[1];ROOT=MOD.parent
base=MOD/'assets-source/czero-zombie';source=base/'zombie_source_FIXED/0/zombie_source.mdl'
d=bytearray(source.read_bytes());assert d[:4]==b'IDST' and struct.unpack_from('<i',d,4)[0]==10
nb,bo=struct.unpack_from('<2i',d,140);nh,ho=struct.unpack_from('<2i',d,156)
hits=[struct.unpack_from('<ii6f',d,ho+i*32) for i in range(nh)];hits=[h for h in hits if h[1]!=8]
assert nb==34 and len(hits)==20
struct.pack_into('<i',d,156,len(hits))
for i,h in enumerate(hits):struct.pack_into('<ii6f',d,ho+i*32,*h)
# CS player poses are centred 36 units above their feet; monsters use feet origin.
assert struct.unpack_from('<i',d,bo+32)[0]==-1
struct.pack_into('<f',d,bo+72,struct.unpack_from('<f',d,bo+72)[0]+36)
ns,so=struct.unpack_from('<2i',d,164)
for i in range(ns):
 for off in (104,116):struct.pack_into('<f',d,so+i*176+off,struct.unpack_from('<f',d,so+i*176+off)[0]+36)
for i in (16,17,18,19,20,21,56,57,60,61,62,63,73,74,75,76):
 desc=so+i*176;blends,anim=struct.unpack_from('<2i',d,desc+120);assert blends in (1,9)
 # Stock HL studio renderers do not evaluate CS's nine-way player blend.
 # Select the centre pitch/yaw pose, retaining the original shooting motion.
 if blends==9:struct.pack_into('<2i',d,desc+120,1,anim+4*nb*12)
gun=next((MOD/'assets-source/leviathan-knife').rglob('p_knife.mdl'));g=gun.read_bytes();gn,gb=struct.unpack_from('<2i',g,140)
weapon_bone=11
assert g[gb+struct.unpack_from('<i',g,gb+weapon_bone*112+32)[0]*112:][:32].split(b'\0')[0]==b'Bip01 R Hand'
local=struct.unpack_from('<3f',g,gb+weapon_bone*112+64)
pistol=bytearray(d)
animation_report=bake_gaits(d)
glock_report=bake_gaits(pistol,19)
na,ao=struct.unpack_from('<2i',d,212);assert na==2
struct.pack_into('<3f',d,ao+40,*local)
out=MOD/'models/cremation/czero';out.mkdir(parents=True,exist_ok=True)
(out/'zombie_knife.mdl').write_bytes(d);shutil.copy2(gun,out/'p_knife.mdl')
(out/'zombie_glock.mdl').write_bytes(pistol)
charred=bytearray(d);nt,to=struct.unpack_from('<2i',charred,180)
for i in range(nt):
 flags,w,h,index=struct.unpack_from('<4i',charred,to+i*80+64);palette=index+w*h
 for c in range(256):
  if flags&64 and c==255:continue
  rgb=charred[palette+c*3:palette+c*3+3];shade=round(sum(rgb)/3*.16)
  charred[palette+c*3:palette+c*3+3]=bytes((shade,shade,shade))
(out/'zombie_knife_charred.mdl').write_bytes(charred)
charred_pistol=bytearray(pistol);nt,to=struct.unpack_from('<2i',charred_pistol,180)
for i in range(nt):
 flags,w,h,index=struct.unpack_from('<4i',charred_pistol,to+i*80+64);palette=index+w*h
 for c in range(256):
  if flags&64 and c==255:continue
  shade=round(sum(charred_pistol[palette+c*3:palette+c*3+3])/3*.16)
  charred_pistol[palette+c*3:palette+c*3+3]=bytes((shade,shade,shade))
(out/'zombie_glock_charred.mdl').write_bytes(charred_pistol)
snd=MOD/'sound/cremation/czero';snd.mkdir(parents=True,exist_ok=True)
for name in ('knife_slash1.wav','knife_hit1.wav','sg_explode.wav'):
 src=next(p for p in (ROOT/f'czero/sound/weapons/{name}',ROOT/f'cstrike/sound/weapons/{name}') if p.exists());shutil.copy2(src,snd/name)
shutil.copy2(ROOT/'czero/models/w_smokegrenade.mdl',out/'w_smokegrenade.mdl')
smoke=MOD/'sprites/cremation/czero';smoke.mkdir(parents=True,exist_ok=True)
# Convert the CS additive puff to index-alpha for opaque grey smoke without
# black rectangular sprite backgrounds in the Half-Life renderer.
raw=bytearray((ROOT/'cstrike/sprites/smokepuff.spr').read_bytes());colors=struct.unpack_from('<H',raw,40)[0]
palette=[raw[42+i*3:45+i*3] for i in range(colors)];struct.pack_into('<i',raw,12,2)
raw[42:42+colors*3]=bytes([160,160,160])*colors;offset=42+colors*3
for frame in range(struct.unpack_from('<i',raw,28)[0]):
 kind,ox,oy,w,h=struct.unpack_from('<5i',raw,offset);assert kind==0;offset+=20
 raw[offset:offset+w*h]=bytes(round(sum(palette[index])/3) for index in raw[offset:offset+w*h]);offset+=w*h
(smoke/'smokepuff.spr').write_bytes(raw)
report=dict(source=str(source),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),folder='0',bones=nb,hitboxes=20,removed_shield_group=8,feet_offset=36,muzzle_bone='Bip01 R Hand',muzzle_local=local,gun_source=str(gun),gait_bakes=animation_report,installed=True)
(base/'installed.json').write_text(json.dumps(report,indent=2)+'\n');print('Installed first-folder zombie and Condition Zero Leviathan knife; body hitboxes and muzzle verified.')
