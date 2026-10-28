# Max (HOTD2)

The monster_max entity reuses the native Ravenholm zombie AI and the David-family damage, hitboxes, wounds, and death handling. Max keeps the native David health, runs continuously, and deals 1.25x David's melee damage. His nine original cha.bin motions are available as separate melee sequences; each has a chainsaw contact event that plays the recovered HOTD2 saw sound.

The actor uses the original 16-bone HOTD2 PC skeleton. The two chainsaws are part of the recovered hand meshes (model IDs 7123 and 7117), not detached props. The five original fall/death clips and original walk/run clips are included. The extracted 29 texture descriptors include 12 textures with alpha; the converter preserves decoded alpha only for materials whose HOTD2 TSP enables an alpha pass, ignores alpha on opaque materials, and emits GoldSrc masked textures.

Max replaces the Quake class in the 10 Quake slots in Rapidcore's main horde. The mansion wave type that previously selected Quake now selects Max. The standalone Quake entity code remains available to older maps, but neither of these horde routes spawns it.

## Rebuild assets

1. Extract the original F*.bin files from the HOTD2 PC DATA.CAB into assets-source/hotd2-max/source/.
2. Run py tools/extract_hotd2_max.py.
3. Run py tools/convert_hotd2_max.py.
4. Run py tools/install_hotd2_max.py to install the assets and replace the existing Rapidcore horde entities.
5. From PowerShell, run source/build_server.ps1 -Install to rebuild and install the proxy.

The extraction manifest, alpha audit, conversion report, original sound, and compiled source assets are under assets-source/hotd2-max/.
