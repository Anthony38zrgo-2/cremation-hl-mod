# Randy

Randy is the original HOTD2 PC armored leaper: skeleton kind 0x19 and geometry/textures zstin.bin (F12999/F13800). The complete helmet, shoulder plates, red chest straps, shorts, exposed knees and claws match the supplied reference. The deformed abdomen is included. His animations are recovered from ans.bin and szom.bin, with shared falling deaths from zom.bin.

monster_randy allocates the native Ravenholm monster_fastzombie. Native AI, schedules, damage, health, collision hull and jump execution remain owned by that class. The model provides 16 original bones and 15 regional studio hitboxes. No David AI adapter is installed on Randy. Legacy monster_fastzombie spawns are also adapted, and legacy fast-zombie models are migrated on restore.

The compiled model preserves all nine native sequence indices, labels, activities, loop flags and event IDs. The run sequence uses Randy's own locomotion motion 0x0A from ans.bin, at its original 52-tick duration, with root travel normalized to the native fast-zombie speed. It does not borrow the unrelated zombie_fast pose. Attack contacts remain events 1 and 2, the leap remains event 4, and climb footsteps retain their original event frames. Other original Randy motions are fitted to those native durations, with root translation owned by entity navigation. Source alpha is decoded per texture and source material; masked GoldSrc surfaces use palette index 255 and STUDIO_NF_MASKED. Actor and charred actor share the same rig.

Randy replaces the ten former fast-zombie slots in Rapidcore and Mansion's fast-zombie composition (20 in wave 3, 10 in wave 4). Ken now participates in the ordinary David-family pool.

Rebuild: tools/extract_hotd2_randy.py, tools/convert_hotd2_randy.py. Install: tools/install_hotd2_randy.py. Model readback: tools/verify_hotd2_randy.py. Real-engine comparison: tools/check_randy_runtime.py. Wave regression: tools/check_mansion_runtime.py. A compiled-model Blender preview is stored with the source assets; this is not an interactive gameplay review.
