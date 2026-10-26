"""Install untouched CZ/CS and DMC HUD atlases without replacing donor HUD."""
from pathlib import Path
import shutil,json,hashlib
MOD=Path(__file__).resolve().parents[1];ROOT=MOD.parent
items=[('cz','640hud7.spr'),('dmc','icnquad.spr'),('dmc','icninvin.spr'),('dmc','icninvis.spr'),('dmc','640hud7.spr')]
rows=[]
for game,name in items:
    source=next(p for p in ([ROOT/'czero/sprites'/name,ROOT/'cstrike/sprites'/name] if game=='cz' else [ROOT/'dmc/sprites'/name]) if p.exists())
    target=MOD/'sprites/cremation/hud'/game/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    rows.append(dict(source=str(source),installed=target.relative_to(MOD).as_posix(),sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
folder=MOD/'assets-source/tactical-hud';folder.mkdir(parents=True,exist_ok=True)
(folder/'installed.json').write_text(json.dumps(rows,indent=2)+'\n')
print('Installed original CSZ/CS bomb/defuse atlas and DMC status sprites, preserving donor HUD.')
