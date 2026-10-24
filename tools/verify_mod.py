"""Static acceptance checks; never starts Half-Life or injects game input."""
from pathlib import Path
import hashlib, json, math, struct
from collections import Counter
from bsp import BSP
from install_horde_assets import add_walk_activity

ROOT=Path(__file__).resolve().parents[2]
MOD=ROOT/'Cremation'
DONOR=ROOT/'Half-Life2Classic_Ravenholm'
source=BSP(ROOT/'valve/maps/rapidcore.bsp')
output=BSP(MOD/'maps/rapidcore.bsp')
report=json.loads((MOD/'placement.json').read_text())
counts=Counter(e['classname'] for e in output.entities)
assert counts['monster_zombie']==20
assert counts['monster_fastzombie']==10 and counts['monster_poisionzombie']==10
assert counts['aiscripted_sequence']==0
assert not any(e['classname']=='trigger_auto' and e.get('target','').startswith('cremation_patrol_') for e in output.entities)
assert sum(e.get('targetname','').startswith('cremation_metal_') for e in output.entities)==6
assert counts['info_player_start']==1 and counts['info_player_deathmatch']==0
assert counts['item_suit']==1 and counts['weapon_ricochet']==1 and counts['weapon_shotgun']==1
assert counts['weapon_bounce_shotgun']==1
assert counts['ammo_9mmclip']==0 and counts['ammo_buckshot']==9
assert {e['classname'] for e in output.entities if e['classname'].startswith('weapon_')}=={'weapon_ricochet','weapon_shotgun','weapon_bounce_shotgun'}
for i in range(1,15): assert source.lump(i)==output.lump(i),f'Geometry lump {i} changed'
spawn=next(e for e in output.entities if e['classname']=='info_player_start')
sp=tuple(map(float,spawn['origin'].split()))
assert output.clear(sp,1)
assert abs(output.floor(*sp[:2])+36-sp[2])<.1
zombies=[e for e in output.entities if e['classname'] in ('monster_zombie','monster_fastzombie','monster_poisionzombie')]
for e in zombies:
    goals=[g for g in output.entities if g.get('netname')==e['targetname']]
    assert len(goals)==2
    initial=tuple(map(float,e['origin'].split()))
    for goal in goals:
        point=tuple(map(float,goal['origin'].split()))
        assert goal['classname']=='info_target' and 'm_iszEntity' not in goal
        assert math.dist(point,initial)<140 and math.dist(point[:2],sp[:2])>800
        assert goal['target'] in {g['targetname'] for g in goals}
    assert any(tuple(map(float,g['origin'].split()))==initial for g in goals)

positions=[]
for e in zombies:
    p=tuple(map(float,e['origin'].split())); positions.append(p)
    assert output.clear((p[0],p[1],p[2]+36),1),e
    assert math.dist(p[:2],sp[:2])>1000,e
    assert abs(output.floor(*p[:2],top=128)+1-p[2])<.1,e
    for height in (0,32,64):
        assert not output.line_clear((sp[0],sp[1],sp[2]+28),(p[0],p[1],p[2]+height)),e
assert min(math.dist(a,b) for i,a in enumerate(positions) for b in positions[i+1:])>96
for e in output.entities:
    if e.get('targetname','').startswith('cremation_loadout_'):
        p=tuple(map(float,e['origin'].split()))
        assert p[:2]==sp[:2] and sp[2]-36<p[2]<sp[2],e
        assert output.clear(p,0),e
# The fast-zombie fix changes one activity integer in place. No table relocation
# after textures: dedicated GoldSrc may omit that part of the model cache.
fast_original=(DONOR/'models/zombie_fast.mdl').read_bytes()
fast=(MOD/'models/zombie_fast.mdl').read_bytes()
assert fast==add_walk_activity(fast_original) and len(fast)==len(fast_original)
assert sum(a!=b for a,b in zip(fast,fast_original))==1
horde_assets=json.loads((MOD/'horde-assets.json').read_text())
entry=horde_assets['models/zombie_fast.mdl']
assert entry['source_sha256']==hashlib.sha256(fast_original).hexdigest()
assert entry['sha256']==hashlib.sha256(fast).hexdigest()
for e in output.entities:
    if e.get('targetname','').startswith('cremation_metal_'):
        assert output.clear(tuple(map(float,e['origin'].split())),0)
import zipfile
with zipfile.ZipFile(MOD/'assets-source/bloodcultisthecu.zip') as archive:
    hecu_assets={}
    prefix='BloodCultistHECU/Cultists models/'
    for name in archive.namelist():
        if name.startswith(prefix) and name.endswith('.mdl'):hecu_assets['models/'+Path(name).name.lower()]=archive.read(name)
        if name.startswith(prefix+'Cultist sounds/') and name.endswith('.wav'):hecu_assets['sound/hgrunt/'+Path(name).name]=archive.read(name)
for relative,raw in hecu_assets.items():assert (MOD/relative).read_bytes()==raw,relative
manifest={}
ricochet_assets=json.loads((MOD/'ricochet-assets.json').read_text())
ricochet_assets.update(json.loads((MOD/'tfc-assets.json').read_text()))
for folder in ('dlls','cl_dlls','models','sound','sprites','events'):
    for path in (DONOR/folder).rglob('*'):
        if not path.is_file(): continue
        relative=path.relative_to(DONOR)
        selected_source=ROOT/ricochet_assets[relative.as_posix()] if relative.as_posix() in ricochet_assets else path
        expected_bytes=add_walk_activity(selected_source.read_bytes()) if relative.as_posix()=='models/zombie_fast.mdl' else selected_source.read_bytes()
        expected_bytes=hecu_assets.get(relative.as_posix().lower(),expected_bytes)
        source_hash=hashlib.sha256(expected_bytes).hexdigest()
        assert hashlib.sha256((MOD/relative).read_bytes()).hexdigest()==source_hash,relative
        manifest[relative.as_posix()]=source_hash
for relative,source in ricochet_assets.items():
    source_hash=hashlib.sha256((ROOT/source).read_bytes()).hexdigest()
    assert hashlib.sha256((MOD/relative).read_bytes()).hexdigest()==source_hash,relative
    manifest[relative]=source_hash
# Bounce Shotgun resources: independent copies and a generated HUD icon.
bounce_assets=json.loads((MOD/'bounce-assets.json').read_text())
for relative,entry in bounce_assets.items():
    path=MOD/relative
    assert path.is_file(),relative
    assert hashlib.sha256(path.read_bytes()).hexdigest()==entry['sha256'],relative
    assert path.stat().st_size==entry['bytes'],relative
    if entry['source'].endswith('.mdl'):
        assert hashlib.sha256((ROOT/entry['source']).read_bytes()).hexdigest()==entry['sha256'],relative
        model_data=path.read_bytes()
        assert model_data[:4]==b'IDST' and struct.unpack_from('<i',model_data,4)[0]==10,relative
        if struct.unpack_from('<i',model_data,180)[0]==0:
            # Mod_LoadStudioModel derives <loaded-name>T.mdl when numtextures=0.
            # A headless server can spawn the map without exercising this client path.
            texture_path=path.with_name(path.stem+'T.mdl')
            assert texture_path.is_file(),f'Missing external model textures: {texture_path}'
            texture_data=texture_path.read_bytes()
            assert texture_data[:4]==b'IDST' and struct.unpack_from('<i',texture_data,4)[0]==10,texture_path
            assert struct.unpack_from('<i',texture_data,180)[0]>0,texture_path
            assert texture_path.relative_to(MOD).as_posix() in bounce_assets,texture_path
    manifest[relative]=entry['sha256']
for relative in ('sprites/cremation_ricochet.spr','sprites/weapon_ricochet.txt','sprites/weapon_9mmhandgun.txt',
                 'sprites/weapon_bounce_shotgun.txt','liblist.gam','cremation.cfg','autoexec.cfg','placement.json',
                 'source/ricochet_proxy.cpp','source/ricochet_balance.h','source/bounce_shotgun.cpp',
                 'source/bounce_shotgun.h','source/bounce_shotgun_balance.h','bounce-assets.json'):
    manifest[relative]=hashlib.sha256((MOD/relative).read_bytes()).hexdigest()
sprite=(MOD/'sprites/cremation_ricochet.spr').read_bytes()
assert sprite[:4]==b'IDSP' and struct.unpack_from('<i',sprite,4)[0]==2
assert struct.unpack_from('<iii',sprite,20)==(128,32,1)
# The new weapon has its own valid HUD sprite: 128x32, one SPR_SINGLE frame
# whose data length matches the header.
bounce_sprite=(MOD/'sprites/cremation_bounce_shotgun.spr').read_bytes()
assert bounce_sprite[:4]==b'IDSP' and struct.unpack_from('<i',bounce_sprite,4)[0]==2
assert struct.unpack_from('<iii',bounce_sprite,20)==(128,32,1)
frame_type,origin_x,origin_y,frame_w,frame_h=struct.unpack_from('<5i',bounce_sprite,42+256*3)
assert frame_type==0 and (frame_w,frame_h)==(128,32)
assert len(bounce_sprite)==42+256*3+20+frame_w*frame_h
descriptor=(MOD/'sprites/weapon_bounce_shotgun.txt').read_text()
assert descriptor.splitlines()[0].strip()=='10' and descriptor.count('cremation_bounce_shotgun')==4
assert not any(line.split()[0]=='ammo2' for line in descriptor.splitlines()[1:] if line.split())
# The weapon must not claim the shotgun/crossbow HUD bucket position and must
# keep its own native carrier id, otherwise the HUD/selection wiring breaks.
balance=(MOD/'source/bounce_shotgun_balance.h').read_text()
for declaration,expected in (('CarrierWeaponId',6),('HudSlot',2),('HudSlotPos',3)):
    line=next(l for l in balance.splitlines() if f'constexpr int {declaration} =' in l)
    assert int(line.split('=')[1].split(';')[0])==expected,(declaration,line)
for relative,source in ricochet_assets.items():
    if relative.endswith('.mdl'):
        data=(MOD/relative).read_bytes()
        assert data[:4]==b'IDST' and struct.unpack_from('<i',data,4)[0]==10
        assert struct.unpack_from('<i',data,172)[0]==1,relative
        assert struct.unpack_from('<i',data,180)[0]>0,relative
assert 'gamedll "dlls/cremation.dll"' in (MOD/'liblist.gam').read_text()
assert 'cl_lw 0' in (MOD/'cremation.cfg').read_text()
import pefile
custom_dll=pefile.PE(str(MOD/'dlls/cremation.dll'))
export_names={e.name.decode() for e in custom_dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
original_dll=pefile.PE(str(MOD/'dlls/hl_ravenholm.dll'))
original_names={e.name.decode() for e in original_dll.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
assert original_names.issubset(export_names)
assert 'weapon_ricochet' in export_names and 'cremation_disc' in export_names
for name in ('weapon_bounce_shotgun','cremation_pellet','cremation_grenade','cremation_sentry','cremation_sentry_base'):
    assert name in export_names,name
assert custom_dll.FILE_HEADER.Machine==0x14c
# Reproduce hw.dll's legacy raw-file parser: it translates every name RVA with
# the .rdata section offset and scans NumberOfNames contiguous C strings.
rdata=next(s for s in custom_dll.sections if s.Name.rstrip(b'\0')==b'.rdata')
directory=custom_dll.DIRECTORY_ENTRY_EXPORT.struct
raw=(MOD/'dlls/cremation.dll').read_bytes()
legacy_offset=lambda rva:rva-rdata.VirtualAddress+rdata.PointerToRawData
table_offset=legacy_offset(directory.AddressOfNames)
first_rva=struct.unpack_from('<I',raw,table_offset)[0]
cursor=legacy_offset(first_rva)
legacy_names=[]
for i in range(directory.NumberOfNames):
    name_rva=struct.unpack_from('<I',raw,table_offset+i*4)[0]
    assert legacy_offset(name_rva)==cursor,'GoldSrc requires contiguous export names'
    end=raw.index(b'\0',cursor)
    assert rdata.PointerToRawData<=cursor<=end<rdata.PointerToRawData+rdata.SizeOfRawData
    legacy_names.append(raw[cursor:end].decode());cursor=end+1
assert set(legacy_names)==export_names
for file in ('Cremation.exe','Cremation/maps/rapidcore.bsp','valve/maps/rapidcore.bsp'):
    path=ROOT/file
    assert path.is_file(),file
    manifest[file]=hashlib.sha256(path.read_bytes()).hexdigest()
assert (ROOT/'Cremation.exe').read_bytes()[:2]==b'MZ'
export_data=(MOD/'dlls/hl.dll').read_bytes()
for name in ('monster_zombie','item_suit','weapon_9mmhandgun','weapon_shotgun','weapon_crossbow','ammo_9mmclip','ammo_buckshot','info_node'):
    assert name.encode()+b'\0' in export_data,name
(MOD/'file-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS: 40 Ravenholm zombies (normal/fast/poison); one clear spawn; suit + Ricochet + shotgun + Bounce Shotgun + buckshot.')
print('PASS: the three weapons are separate BSP entities with separate HUD slots.')
print('PASS: standing hulls, floor contact, spacing >96, start distance >1000, no direct visibility.')
print('PASS: all 14 non-entity BSP lumps unchanged; donor assets + overrides match their sources.')
print('PASS: Win32 Cremation DLL preserves all Ravenholm exports and adds both custom weapons.')
print('PASS: GoldSrc legacy .rdata export-name reader resolves every contiguous name safely.')
print('PASS: Bounce Shotgun models are private copies; HUD sprite and descriptor are valid.')
print('PASS: models with external textures have the matching <model-name>T.mdl companion.')
print(f'PASS: {len(manifest)} file hashes recorded; launcher PE header present.')
print('Manual gameplay validation remains pending.')

hecu=[e for e in output.entities if e.get('targetname','').startswith('cremation_hecu_')]
assert len(hecu)==12,len(hecu)
assert {int(e['weapons']) for e in hecu}=={1,8,3,10,5,12}
for e in hecu:
    x,y,z=map(float,e['origin'].split())
    assert output.clear((x,y,z+36),1)
    assert math.dist((x,y),sp[:2])>=1050
print('PASS: twelve Blood Cultist HECU with six native weapon loadouts.')
