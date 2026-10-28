# Judgment model assets

This folder contains the extracted HOTD2 Judgment resources and the GoldSrc assembly used by Cremation.

## Built models

- `kuarl.mdl`: Kuarl's `0x44` rig, all 18 original motions from `boss1.bin` (`0x383–0x394`), the original node-2 hide override, and the extra model-gallery attachment `667`.
- `zeal.mdl`: Zeal's `0x45` body rig plus the synchronized `0x46` wing rig, all 15 original body motions (`0x408–0x416`), and the original attachments `674`, `708`, and `707`. Wing motion `0x14` accompanies charge `0x40C`; `0x10` accompanies the other clips.
- The source textures are decoded to RGBA PNGs. Materials that use the source alpha pass are converted to GoldSrc's 8-bit masked palette convention and their compiled MDL texture records carry `STUDIO_NF_MASKED`.

The compiled files are installed at `models/cremation/hotd2/judgment/`. QC, SMD, indexed BMPs, extraction/build scripts, and conversion reports remain under `goldsrc/` and this folder so the assets can be rebuilt.

Kuarl is built at 280 units tall, 2.5 times the initial 112-unit assembly. His animated root already carries the pelvis height above the ground. The converter adds the reference-pose floor offset only to Zeal; adding it again to Kuarl's motions lifts his feet above his collision hull. Hitboxes are generated from each visible bone's local mesh bounds so they follow the animated body, legs and wings.

The wave director waits for all 80 regular wave-4 enemies to die before emitting Judgment. Zeal orbits the upper part of Kuarl while both are alive. A reliable six-byte `JudgmentHUD` snapshot supplies shared HP to the centered red client health bar and selects `sound/music/judgment_leprosy.wav` during the encounter. `tools/install_judgment_music.py` converts the supplied Leprosy MP3 into the stereo 44.1-kHz PCM format used by the existing music streamer; `music.json` records provenance.

`python tools/check_judgment_runtime.py` uses the validation DLL and a real HLDS fake client to check route movement, grounded animated feet, Zeal's vertical flight, shared damage, the 1500/450 HP phase change and death animation advancement. Visual acceptance during interactive combat remains a separate check.

Judgment overrides the inherited Gargantua bullet filter through the Controller's ordinary monster `TraceAttack`, then applies the shared-health armor/weak-point rules. Its server broad-phase bounds enclose the rotated MDL sequence bounds, including Zeal's wings; they do not inherit Gargantua's fixed 214-unit box or Controller's narrow default hull.

The flight controller sweeps Zeal's complete 96x96x76 navigation volume, rejects enclosed destinations and keeps checked intermediate waypoints when passing above an obstacle. It uses local steering when no complete detour is available. The runtime fixture checks 128 actual engine rays through authored body/head/wing poses, a solo-Zeal defeat through `TraceAttack` plus multi-damage, and travel around the mansion with no obstructed flight-volume samples. `python tools/generate_judgment_probe_poses.py` regenerates the independent pose targets before rebuilding the validation DLL after asset changes; engine `GET_BONE_POSITION` supplies positions but does not supply usable bone angles in this runtime.

## Rebuild

Run `python tools/extract_hotd2_judgment.py`, then `python tools/convert_hotd2_judgment.py` from the Cremation repository. The scripts read `D:\games\House_of_the_dead_2_ISO\DATA.CAB` and `HOD2.EXE`, use the shared parser package under `David_analysis/hotd2-re-reference/tools`, and compile with the installed Half-Life SDK `studiomdl.exe`.

`*_conversion_report.json` records source motion IDs, compiled bones/sequences/textures, source-alpha counts, and the installed MDL SHA-256. The preview images are a Blender render of a representative animated frame for visual assembly inspection; verify final behavior, hit registration, and animation selection after the boss entities are integrated into gameplay.
