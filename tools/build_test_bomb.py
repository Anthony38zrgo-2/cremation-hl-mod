"""R5b-test: 1 CZ maker sobre la zona A + zonas de plantado."""
import sys
sys.path.insert(0, "tools")
from bsp import BSP

bsp = BSP("backups/rewrite-maps/cs_mansion.bsp")
out = [dict(e) for e in bsp.entities]
out.append({"classname": "info_target", "targetname": "cz_plant_a",
            "origin": "-304 -272 56"})
out.append({"classname": "info_target", "targetname": "cz_plant_b",
            "origin": "-624 -176 192"})
out.append({"classname": "monstermaker", "targetname": "bomb_test_maker",
            "origin": "-304 -272 80", "monstertype": "monster_czero_zombie",
            "monstercount": "1", "m_imaxlivechildren": "1",
            "spawnflags": "1114145", "spawndelay": "1",
            "ent_template": "monster_czero_zombie",
            "#weapons": "1", "#freeroam": "1"})
bsp.write("maps/test_bomb.bsp", out)
print("test_bomb.bsp:", len(out))
