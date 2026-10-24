"""Install the supplied Blood Cultist HECU model, only in Cremation."""
from pathlib import Path
import zipfile, hashlib, json
ROOT=Path(__file__).resolve().parents[2]
MOD=ROOT/'Cremation'
def install():
    archive=MOD/'assets-source/bloodcultisthecu.zip'
    with zipfile.ZipFile(archive) as z:
        prefix='BloodCultistHECU/Cultists models/'
        for name in ('hgrunt.mdl','hgrunt01.mdl','hgrunt02.mdl','hgrunt03.mdl','hgruntT.mdl'):
            (MOD/'models'/name.lower()).write_bytes(z.read(prefix+name))
        out=MOD/'sound/hgrunt';out.mkdir(parents=True,exist_ok=True)
        for entry in z.namelist():
            if entry.startswith(prefix+'Cultist sounds/') and entry.endswith('.wav'):
                (out/Path(entry).name).write_bytes(z.read(entry))
if __name__=='__main__':install()
