"""Install the supplied normal-zombie skin without changing fast/poison models."""
from pathlib import Path
import hashlib, json, shutil, struct, subprocess

MOD=Path(__file__).resolve().parents[1]
SOURCE=MOD/'assets-source/purple-zombie'

def install():
    SOURCE.mkdir(parents=True,exist_ok=True)
    archive=MOD/'assets-source/purple_zombie.rar'
    if not archive.exists():shutil.copy2(Path.home()/'Downloads/purple_zombie.rar',archive)
    manifest={}
    backup=MOD/'backups/before-purple-zombies';backup.mkdir(parents=True,exist_ok=True)
    for name in ('zombie.mdl','headcrab.mdl'):
        data=subprocess.check_output(['tar','-xOf',str(archive),'purple zombie/'+name])
        assert data[:4]==b'IDST' and struct.unpack_from('<i',data,4)[0]==10
        assert struct.unpack_from('<i',data,180)[0]>0,'Model needs external textures'
        (SOURCE/name).write_bytes(data)
        dest=MOD/'models'/name
        old=backup/('models_'+name)
        if dest.exists() and not old.exists():shutil.copy2(dest,old)
        dest.write_bytes(data)
        relative=dest.relative_to(MOD).as_posix()
        manifest[relative]=dict(source=f'assets-source/purple_zombie.rar:purple zombie/{name}',sha256=hashlib.sha256(data).hexdigest())
        if name=='zombie.mdl':
            # Burning corpses must keep exactly the new model's skeleton and
            # sequence table, rather than switching back to the old zombie rig.
            charred=bytearray(data);count,offset=struct.unpack_from('<ii',data,180)
            for i in range(count):
                tex=offset+i*80;w,h,pixels=struct.unpack_from('<iii',data,tex+68)
                palette=pixels+w*h
                charred[palette:palette+768]=bytes(round(v*.73) for v in data[palette:palette+768])
            dest=MOD/'models/cremation/charred/zombie.mdl';old=backup/'models_cremation_charred_zombie.mdl'
            if dest.exists() and not old.exists():shutil.copy2(dest,old)
            dest.write_bytes(charred)
            manifest[dest.relative_to(MOD).as_posix()]=dict(source='models/zombie.mdl',transform='palette x 0.73; skeleton/animation unchanged',sha256=hashlib.sha256(charred).hexdigest())
    (MOD/'purple-zombie-assets.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Installed purple normal zombie, associated normal headcrab and matching burned-corpse model.')

if __name__=='__main__':
    from install_hotd2_max import replace_map
    # Back up map/model state before installing the replacement assets.
    replace_map()
    install()
