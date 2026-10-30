"""R3: inyecta el director de oleadas en los BSP de cs_mansion.

Por cada mapa (cs_mansion wave1, cs_mansion_wave1..4) inserta:
- 1 monstermaker por monstruo (distribuidos en deathmatch + mansion_ct_*),
  con ent_template / child keys (roam, trigger, weapons, PVS, non-solid).
- trigger_relay + game_counter que cuenta muertes de la oleada.
- env_message de "OLEADA N ELIMINADA" al completar.
- Loadout inicial junto a cada info_player_start.

Composicion portada de source/mansion_waves.h (kind/baseKind/isTpose).
Uso:  python tools/build_mansion_director.py [--write]
Sin --write solo informa. Con --write guarda backup .bak y reescribe el BSP.
"""
import sys
sys.path.insert(0, "tools")
from bsp import BSP

WAVES = {  # bsp base (sin sufijo) = oleada 1
    "maps/cs_mansion.bsp": 1,
    "maps/cs_mansion_wave1.bsp": 1,
    "maps/cs_mansion_wave2.bsp": 2,
    "maps/cs_mansion_wave3.bsp": 3,
    "maps/cs_mansion_wave4.bsp": 4,
}

CLASSNAMES = {
    0: "monster_zombie",
    1: "monster_max",
    2: "monster_randy",
    4: "monster_human_grunt",
    5: "monster_czero_zombie",
    6: "monster_tpose",
}
TEMPLATES = {0: "david_base", 4: "blood_cultist_hecu"}
WEAPONS = {5: "1"}  # CZ: 9mmAR de momento


def base_kind(wave, index):
    sector, rnd = index % 4, index // 4
    if wave == 1:
        return 0
    if wave == 2:
        return (rnd + sector) % 2
    if wave == 3:
        return (rnd + sector) % 4
    return -1 if rnd % 2 else (rnd // 2 + sector) % 4  # wave4: -1 = grunt


def is_tpose(wave, index, total):
    if index >= total:
        return False
    if wave == 2:
        return index in (18, 23, 29)
    if wave in (3, 4):
        return base_kind(wave, index) == 3
    return False


def kind(wave, index):
    total = 40 if wave <= 2 else 80
    if index == total:
        return 5
    if wave <= 3 and index == 0:
        return 5
    if is_tpose(wave, index, total):
        return 6
    b = base_kind(wave, index)
    return 4 if b == -1 else b


def composition(wave):
    total = 40 if wave <= 2 else 80
    if wave == 4:
        total += 1
    return [kind(wave, i) for i in range(total)]


LOADOUT = [
    ("weapon_crowbar", None),
    ("weapon_9mmhandgun", None),
    ("weapon_shotgun", None),
    ("ammo_9mmclip", None),
    ("ammo_buckshot", None),
    ("ammo_buckshot", None),
    ("item_suit", None),
    ("item_battery", None),
]

MAKER_FLAGS = 1 + 32 + 65536 + 1048576  # Start ON + AutoBBox + Non-solid + PVS


def build(wave, entities):
    spots = [e["origin"] for e in entities
             if e["classname"] == "info_player_deathmatch" and "origin" in e]
    spots += [e["origin"] for e in entities
              if e.get("targetname", "").startswith("mansion_ct_") and "origin" in e]
    assert spots, "sin puntos de spawn"
    comp = composition(wave)
    assert all(k in CLASSNAMES for k in comp), comp
    total = len(comp)
    out = [dict(e) for e in entities]
    for i, k in enumerate(comp):
        maker = {
            "classname": "monstermaker",
            "targetname": f"wave{wave}_maker_{i:02d}",
            "origin": spots[i % len(spots)],
            "monstertype": CLASSNAMES[k],
            "monstercount": "1",
            "m_imaxlivechildren": "1",
            "spawnflags": str(MAKER_FLAGS),
            "spawndelay": f"{(i % 10) * 1.5:.1f}",
            "trigger_on_death_notice": f"wave{wave}_dead",
            "face_position": "*player",
            "#freeroam": "1",
            "#triggercondition": "4",
            "#triggertarget": f"wave{wave}_dead",
        }
        if k in TEMPLATES:
            maker["ent_template"] = TEMPLATES[k]
        if k in WEAPONS:
            maker["#weapons"] = WEAPONS[k]
        out.append(maker)
    out.append({"classname": "trigger_relay", "targetname": f"wave{wave}_dead",
                "target": f"wave{wave}_counter"})
    out.append({"classname": "game_counter", "targetname": f"wave{wave}_counter",
                "target": f"wave{wave}_done", "frags": "0",
                "health": str(total)})
    out.append({"classname": "env_message", "targetname": f"wave{wave}_done",
                "message": f"OLEADA {wave} ELIMINADA ({total})"})
    for e in entities:
        if e["classname"] == "info_player_start" and "origin" in e:
            for cls, _ in LOADOUT:
                out.append({"classname": cls, "origin": e["origin"]})
    return out


def main():
    write = "--write" in sys.argv
    for path, wave in WAVES.items():
        bsp = BSP(path)
        comp = composition(wave)
        print(f"{path}: oleada {wave}, {len(comp)} monstruos")
        if not write:
            continue
        ents = build(wave, bsp.entities)
        bsp.write(path + ".new", ents)
        print(f"  escrito {path}.new ({len(ents)} entidades)")
    if not write:
        print("solo informe; usa --write para generar .new")


if __name__ == "__main__":
    main()
