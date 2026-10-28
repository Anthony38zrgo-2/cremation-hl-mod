# Ken

Ken is the HOTD2 PC skeleton kind 0x0D, model/texture resource znkagex.bin. The recovered character was identified from the user reference as Ken.

Entity monster_ken allocates the native Ravenholm zombie and uses the David-family navigation, crowd running rules, ordinary health, death handling and equal attack selection. Its melee contacts apply exactly 2x David damage through the existing victim damage wrappers. Ken has saved variant 4, an independent model, 15 regional hitboxes and original skag.bin strikes.

The 13 compiled sequences follow the standing David layout (idle/walk/run, three strikes, two pain reactions, five real falling deaths). Source geometry includes the deformable abdomen and the original claw meshes. The hair textures 1 and 2 retain their decoded alpha in every material pass; GoldSrc palette index 255 and STUDIO_NF_MASKED are applied to actor, charred actor and body pieces.

Ken is distributed among David-family zombies through cremation_ken_chance (default one third of non-Johnny ordinary zombies). With the default Steve chance, David, Steve and Ken share that pool equally. Randy occupies the fast-zombie slots. Ken can also be spawned independently as monster_ken.

Rebuild assets with tools/extract_hotd2_ken.py and tools/convert_hotd2_ken.py. Install with tools/install_hotd2_ken.py. The main asset build also calls the Ken installer.
