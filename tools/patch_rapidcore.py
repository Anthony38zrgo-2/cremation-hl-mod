"""R3: inyecta loadout inicial en rapidcore (el proxy lo daba por codigo)."""
import sys
sys.path.insert(0, "tools")
from bsp import BSP

LOADOUT = ["weapon_crowbar", "weapon_9mmhandgun", "weapon_shotgun",
           "ammo_9mmclip", "ammo_buckshot", "ammo_buckshot", "item_suit",
           "item_battery"]

bsp = BSP("maps/rapidcore.bsp")
starts = [e for e in bsp.entities if e["classname"] == "info_player_start"]
print("starts:", len(starts))
out = [dict(e) for e in bsp.entities]
have = {e["classname"] for e in out}
for e in starts:
    for cls in LOADOUT:
        out.append({"classname": cls, "origin": e["origin"]})
if "--write" in sys.argv:
    bsp.write("maps/rapidcore.bsp.new", out)
    print("escrito maps/rapidcore.bsp.new:", len(out))
else:
    print("solo informe; usa --write")
