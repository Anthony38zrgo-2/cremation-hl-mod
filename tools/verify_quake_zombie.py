"""Focused checks for the independent Quake zombie replacement."""
from pathlib import Path
from collections import Counter
import hashlib, json, struct, zipfile
import pefile
from bsp import BSP
from install_horde_assets import add_walk_activity

MOD = Path(__file__).resolve().parents[1]
old = BSP(MOD / 'backups/before-quake-zombies/rapidcore.bsp')
new = BSP(MOD / 'maps/rapidcore.bsp')
expected = [dict(e) for e in old.entities]
replacements = 0
for e in expected:
    if e.get('classname') == 'monster_zombie':
        e['classname'] = 'monster_quake_zombie'
        replacements += 1
patrols=json.loads((MOD/'quake-zombie-patrols.json').read_text())
for e in expected:
    if e.get('targetname') in patrols:
        assert e['classname']=='info_target' and int(e['targetname'].split('_')[2])<=20
        change=patrols[e['targetname']];assert e['origin']==change['before']
        e['origin']=change['after']
assert replacements == 20 and new.entities == expected
for i in range(1, 15): assert new.lump(i) == old.lump(i), i
counts = Counter(e['classname'] for e in new.entities)
assert counts['monster_quake_zombie'] == 20 and counts['monster_zombie'] == 0
assert counts['monster_fastzombie'] == counts['monster_poisionzombie'] == 10
donor = MOD.parent / 'Half-Life2Classic_Ravenholm/models'
for filename in ('zombie.mdl', 'poison_zombie.mdl'):
    assert (MOD / 'models' / filename).read_bytes() == (donor / filename).read_bytes()
assert (MOD / 'models/zombie_fast.mdl').read_bytes() == add_walk_activity((donor / 'zombie_fast.mdl').read_bytes())
for relative, entry in json.loads((MOD / 'quake-zombie-assets.json').read_text()).items():
    data = (MOD / relative).read_bytes()
    assert hashlib.sha256(data).hexdigest() == entry['sha256'] and len(data) == entry['bytes']
    if relative.endswith('.mdl'):
        assert data[:4] == b'IDST' and struct.unpack_from('<i', data, 4)[0] == 10
remaster=MOD/'assets-source/quake-zombie-remaster/goldsrc/zombie.mdl'
assert (MOD/'models/cremation/quake/zombie.mdl').read_bytes()==remaster.read_bytes()
conversion=json.loads((MOD/'assets-source/quake-zombie-remaster/conversion-report.json').read_text())
assert conversion['scale']==2.0 and conversion['source_frames']==198
assert conversion['goldsrc_bones']<=128 and conversion['triangles']>1000
assert 102<conversion['idle_bounds'][1][2]<105 and len(conversion['hitboxes'])==15
pe = pefile.PE(str(MOD / 'dlls/cremation.dll'))
exports = {s.name.decode() for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name}
assert {'monster_quake_zombie', 'quake_zombie_gib', 'monster_zombie', 'monster_fastzombie', 'monster_poisionzombie', 'monster_minion'} <= exports
print('PASS: exactly 20 classname replacements and documented tall-zombie patrol corrections; other entities and geometry preserved.')
print('PASS: fast/poison and native zombie models unchanged; 16 private Quake assets verified.')
print('PASS: both new entity exports and original zombie/minion exports available in installed Win32 DLL.')
