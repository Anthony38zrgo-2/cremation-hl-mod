"""Install original DMC artifacts, texture companions and CZ bomb resources."""
from pathlib import Path
import shutil,json,hashlib
MOD=Path(__file__).resolve().parents[1];ROOT=MOD.parent
out=MOD/'models/cremation/dmc';out.mkdir(parents=True,exist_ok=True);report=[]
for stem in ('pow_quad','pow_invuln','pow_invis','suit'):
    for src in (ROOT/'dmc/models').glob(stem+'*.mdl'):
        shutil.copy2(src,out/src.name);report.append(dict(source=str(src),installed=str((out/src.name).relative_to(MOD)),sha256=hashlib.sha256(src.read_bytes()).hexdigest()))
snd=MOD/'sound/cremation/dmc';snd.mkdir(parents=True,exist_ok=True)
for stem in ('damage','protect','inv','suit'):
    for src in (ROOT/'dmc/sound/items').glob(stem+'*.wav'):shutil.copy2(src,snd/src.name)
for name in ('w_c4.mdl','w_flashbang.mdl','p_glock18.mdl'):
    src=next(p for p in (ROOT/'czero/models'/name,ROOT/'cstrike/models'/name) if p.exists());shutil.copy2(src,MOD/'models/cremation/czero'/name)
for name in ('flashbang-1.wav','c4_beep1.wav','c4_explode1.wav','c4_plant.wav','c4_disarm.wav','c4_disarmed.wav','glock18-2.wav'):
    src=next((p for p in (ROOT/'czero/sound/weapons'/name,ROOT/'cstrike/sound/weapons'/name) if p.exists()),None)
    if src:shutil.copy2(src,MOD/'sound/cremation/czero'/name)
folder=MOD/'assets-source/dmc-powerups';folder.mkdir(parents=True,exist_ok=True)
(folder/'installed.json').write_text(json.dumps(report,indent=2)+'\n')
assert all((out/(n+'.mdl')).exists() for n in ('pow_quad','pow_invuln','pow_invis','suit'))
print('Installed four original DMC artifacts and CZ bomb/flash resources.')
