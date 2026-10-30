"""R5: mapa de prueba con las 6 armas + municion en el spawn."""
import sys
sys.path.insert(0, "tools")
from bsp import BSP

WEAPONS = ["weapon_pipewrench", "weapon_nailgun", "weapon_shockrifle",
           "weapon_eagle", "weapon_shotgun2", "weapon_sniperrifle2"]
AMMO = ["ammo_nails", "ammo_cells", "ammo_357", "ammo_buckshot", "ammo_762",
        "ammo_nails", "ammo_cells", "ammo_buckshot"]

bsp = BSP("maps/rapidcore.bsp")
starts = [e for e in bsp.entities if e["classname"] == "info_player_start"]
out = [dict(e) for e in bsp.entities]
for e in starts:
    for cls in WEAPONS + AMMO:
        out.append({"classname": cls, "origin": e["origin"]})
bsp.write("maps/test_weapons.bsp", out)
print("test_weapons.bsp:", len(out), "entidades")
