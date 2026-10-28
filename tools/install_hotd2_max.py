"""Install recovered HOTD2 Max assets and replace the Quake horde slots."""
from pathlib import Path
import hashlib, json, shutil

MOD = Path(__file__).resolve().parents[1]
SOURCE = MOD / "assets-source/hotd2-max"
GOLDSRC = SOURCE / "goldsrc"
DEST = MOD / "models/cremation/hotd2/max"
SOUND_SOURCE = SOURCE / "source/F13888_chain_saw_22.wav"
SOUND_DEST = MOD / "sound/cremation/hotd2/max/chain_saw_22.wav"


def main_zombie_class(index):
    """Alternate ten Maxes with ten David-family zombies in the main horde."""
    assert 0 <= index < 20
    return "monster_max" if index % 2 == 0 else "monster_zombie"


def install():
    actor = GOLDSRC / "max.mdl"
    charred = GOLDSRC / "max_charred.mdl"
    parts = sorted(GOLDSRC.glob("part_*.mdl"))
    if not actor.is_file() or not charred.is_file() or len(parts) != 13:
        raise FileNotFoundError("Run tools/convert_hotd2_max.py first; expected Max actor, charred actor and 13 regional meshes.")
    if not SOUND_SOURCE.is_file():
        raise FileNotFoundError(SOUND_SOURCE)
    DEST.mkdir(parents=True, exist_ok=True)
    manifest = {}
    for source in [actor, charred, *parts]:
        target = DEST / source.name
        shutil.copy2(source, target)
        manifest[target.relative_to(MOD).as_posix()] = {
            "source": source.relative_to(MOD).as_posix(),
            "sha256": hashlib.sha256(target.read_bytes()).hexdigest(),
        }
    SOUND_DEST.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(SOUND_SOURCE, SOUND_DEST)
    manifest[SOUND_DEST.relative_to(MOD).as_posix()] = {
        "source": SOUND_SOURCE.relative_to(MOD).as_posix(),
        "sha256": hashlib.sha256(SOUND_DEST.read_bytes()).hexdigest(),
    }
    (MOD / "hotd2-max-assets.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Installed Max actor, charred actor, {len(parts)} body parts and original chainsaw sound.")


def replace_map():
    from bsp import BSP

    path = MOD / "maps/rapidcore.bsp"
    if not path.is_file():
        raise FileNotFoundError(path)
    bsp = BSP(path)
    main = [entity for entity in bsp.entities
            if entity.get("targetname", "").startswith("cremation_zombie_")
            and int(entity["targetname"].rsplit("_", 1)[1]) <= 20]
    if len(main) != 20:
        raise RuntimeError(f"Expected 20 main-horde placements in Rapidcore, found {len(main)}.")
    backup = MOD / "backups/before-hotd2-max"
    backup.mkdir(parents=True, exist_ok=True)
    for source in (path, MOD / "placement.json"):
        if source.is_file():
            target = backup / source.name
            if not target.exists():
                shutil.copy2(source, target)
    changes = 0
    for entity in main:
        index = int(entity["targetname"].rsplit("_", 1)[1]) - 1
        desired = main_zombie_class(index)
        if entity["classname"] != desired:
            entity["classname"] = desired
            changes += 1
    if changes:
        bsp.write(path, bsp.entities)
    report_path = MOD / "placement.json"
    if report_path.is_file():
        report = json.loads(report_path.read_text())
        for placement in report.get("zombies", []):
            if placement.get("id", 0) <= 20:
                placement["classname"] = main_zombie_class(placement["id"] - 1)
        report_path.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Rapidcore main horde: 10 Max + 10 David-family zombies ({changes} class replacements).")


if __name__ == "__main__":
    install()
    replace_map()
