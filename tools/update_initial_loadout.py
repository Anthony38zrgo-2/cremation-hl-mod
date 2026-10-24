"""Apply the configured startup inventory to the installed Rapidcore BSP only."""
import json
from bsp import BSP
from build_mod import LOADOUT, MOD

MAP = MOD / "maps" / "rapidcore.bsp"
PLACEMENT = MOD / "placement.json"


def update():
    bsp = BSP(MAP)
    old = [e for e in bsp.entities if e.get("targetname", "").startswith("cremation_loadout_")]
    if not old:
        raise RuntimeError("Rapidcore BSP has no existing Cremation startup loadout to anchor")
    origin = old[0]["origin"]
    angles = old[0].get("angles", "0 155 0")
    entities = [e for e in bsp.entities if not e.get("targetname", "").startswith("cremation_loadout_")]
    for index, classname in enumerate(LOADOUT, 1):
        entities.append({
            "classname": classname,
            "origin": origin,
            "angles": angles,
            "targetname": f"cremation_loadout_{index:02}",
        })
    bsp.write(MAP, entities)

    placement = json.loads(PLACEMENT.read_text(encoding="utf-8"))
    placement["loadout"] = LOADOUT
    PLACEMENT.write_text(json.dumps(placement, indent=2) + "\n", encoding="utf-8")
    print(f"Updated {MAP.name} and placement.json with {len(LOADOUT)} startup inventory entities.")


if __name__ == "__main__":
    update()
