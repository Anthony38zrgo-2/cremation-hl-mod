"""Install the user-requested Opposing Force, Nailgun, Hive Hand and grenade assets."""
from pathlib import Path
import hashlib
import json
import shutil

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / "Cremation"
SOURCE = MOD / "assets-source" / "additional_weapons"
BACKUP = MOD / "backups" / "before-sound-transfer-fix-20261024"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def mappings():
    out = {}
    for name in ("v_grenade.mdl", "p_grenade.mdl", "w_grenade.mdl", "w_grenadet.mdl"):
        out[f"models/{name}"] = SOURCE / "granada_tnt_by_ben" / name

    for name, target in (("v_hgun.mdl", "v_9mmhandgun.mdl"),
                         ("p_hgun.mdl", "p_9mmhandgun.mdl"),
                         ("w_hgun.mdl", "w_9mmhandgun.mdl")):
        out[f"models/{target}"] = SOURCE / "hive_hand_original" / name

    for name in ("v_pipe_wrench.mdl", "p_pipe_wrench.mdl", "w_pipe_wrench.mdl"):
        out[f"models/cremation/{name}"] = SOURCE / "opposing_force_pipe_wrench" / name
    wrench_sounds = (
        "pwrench_big_hit1.wav", "pwrench_big_hit2.wav", "pwrench_big_hitbod1.wav",
        "pwrench_big_hitbod2.wav", "pwrench_big_miss.wav", "pwrench_hit1.wav",
        "pwrench_hit2.wav", "pwrench_hitbod1.wav", "pwrench_hitbod2.wav",
        "pwrench_hitbod3.wav", "pwrench_miss1.wav", "pwrench_miss2.wav",
    )
    for name in wrench_sounds:
        out[f"sound/weapons/{name}"] = SOURCE / "opposing_force_pipe_wrench" / "sound" / name
    for name in ("weapon_pipewrench.txt", "320hudof01.spr", "640hudof01.spr", "640hudof02.spr"):
        out[f"sprites/{name}"] = SOURCE / "opposing_force_pipe_wrench" / "sprites" / name

    for name in ("v_nailgun.mdl", "p_nailgun.mdl", "nail.mdl"):
        out[f"models/cremation/{name}"] = SOURCE / "nailgun" / name
    for name in ("spike1.wav", "spike2.wav"):
        out[f"sound/weapons/{name}"] = SOURCE / "nailgun" / "sound" / name
    for name in ("weapon_nailgun.txt", "tfchud07.spr", "320hud2.spr", "640hud7.spr", "crosshairs.spr"):
        out[f"sprites/{name}"] = SOURCE / "nailgun" / "sprites" / name
    # Explicit later user overrides take precedence over the original assets.
    out["models/v_9mmhandgun.mdl"] = SOURCE / "ricochet_variants" / "v_ricochet_blue.mdl"
    out["models/cremation/v_ricochet_red.mdl"] = SOURCE / "ricochet_variants" / "v_ricochet_red.mdl"
    out["models/cremation/v_nailgun.mdl"] = SOURCE / "unreal_hive_hand" / "v_hgun_mp5_anims.mdl"
    out["models/cremation/link_skeleton.mdl"] = MOD / "assets-source" / "linkgun_skeleton" / "scientist.mdl"
    out["models/cremation/nail.mdl"] = MOD / "assets-source" / "linkgun_projectile" / "green_oval.mdl"
    chainsaw = SOURCE / "chainsaw" / "Half-Life" / "gearbox"
    for name in ("v_pipe_wrench.mdl", "p_pipe_wrench.mdl", "w_pipe_wrench.mdl"):
        out[f"models/cremation/{name}"] = chainsaw / "models" / name
    for path in (chainsaw / "sound").rglob("*.wav"):
        out["sound/" + path.relative_to(chainsaw / "sound").as_posix()] = path
    for path in (chainsaw / "sprites").glob("*.spr"):
        out["sprites/" + path.name] = path
    for name in ("ar2_fire.wav", "ar2_altfire.wav", "energy_sing_explosion2.wav",
                 "energy_bounce1.wav", "energy_bounce2.wav"):
        out[f"sound/weapons/{name}"] = SOURCE / "missing_sound_fallbacks" / name
    return out


def install() -> dict:
    from prepare_ricochet_model import build_assets
    from prepare_nailgun_animation import build_assets as build_nailgun
    build_assets()
    build_nailgun()
    from prepare_linkgun_projectile import build_assets as build_projectile
    build_projectile()
    from prepare_link_plasma_sprites import build_assets as build_plasma
    build_plasma()
    installed = {}
    for target_name, source_path in mappings().items():
        if not source_path.is_file():
            raise FileNotFoundError(f"Missing requested weapon source asset: {source_path}")
        target = MOD / Path(target_name)
        target.parent.mkdir(parents=True, exist_ok=True)
        old_hash = sha256(target) if target.is_file() else None
        new_hash = sha256(source_path)
        backup_rel = None
        if old_hash and old_hash != new_hash:
            backup = BACKUP / Path(target_name)
            if not backup.exists():
                backup.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(target, backup)
            backup_rel = backup.relative_to(MOD).as_posix()
        shutil.copy2(source_path, target)
        installed[target_name] = {
            "source": source_path.relative_to(MOD).as_posix(),
            "source_sha256": new_hash,
            "sha256": new_hash,
            "bytes": target.stat().st_size,
            "backup": backup_rel,
        }

    (MOD / "additional-weapons-assets.json").write_text(
        json.dumps({
            "source_archive": "C:/Users/bill/Downloads/graneda_tnt_by_ben.rar",
            "later_overrides": ["d3_chainsaw.zip", "panthergun2.zip:v_HGun_blue.mdl"],
            "sound_fallbacks": {
                "weapons/ar2_fire.wav": "weapons/glauncher.wav",
                "weapons/ar2_altfire.wav": "weapons/glauncher2.wav",
                "weapons/energy_sing_explosion2.wav": "weapons/explode3.wav",
                "weapons/energy_bounce1.wav": "weapons/ric1.wav",
                "weapons/energy_bounce2.wav": "weapons/ric2.wav",
            },
            "archive_contents_used": [
                "v_grenade.mdl", "p_grenade.mdl", "w_grenade.mdl", "w_grenadet.mdl"
            ],
            "installed_targets": installed,
        }, indent=2) + "\n",
        encoding="utf-8",
    )

    manifest_path = MOD / "file-manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8")) if manifest_path.exists() else {}
    for target_name, info in installed.items():
        manifest[target_name] = info["sha256"]
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return installed


if __name__ == "__main__":
    installed = install()
    print(f"Installed {len(installed)} user-requested weapon and HUD assets.")
