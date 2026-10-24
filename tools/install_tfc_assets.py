from pathlib import Path
import shutil,json,hashlib
ROOT=Path(__file__).resolve().parents[2]; MOD=ROOT/'Cremation'
COPIES={
 'models/v_shotgun.mdl':'tfc/models/v_tfc_shotgun.mdl',
 'models/p_shotgun.mdl':'tfc/models/p_smallshotgun.mdl',
 'models/cremation/tfc_sentry.mdl':'tfc/models/sentry1.mdl',
 'models/cremation/tfc_sentry_base.mdl':'tfc/models/base.mdl',
 'sound/cremation/tfc_shotgun.wav':'tfc/sound/weapons/shotgn2.wav',
 'sound/cremation/tfc_sentry_fire.wav':'tfc/sound/weapons/sgun1.wav',
 'sound/cremation/tfc_build.wav':'tfc/sound/weapons/building.wav',
}
def install():
 for dest,src in COPIES.items():
  p=MOD/dest;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/src,p)
 (MOD/'tfc-assets.json').write_text(json.dumps(COPIES,indent=2)+'\n')
 print('Installed TFC shotgun and sentry assets.')
if __name__=='__main__':install()
