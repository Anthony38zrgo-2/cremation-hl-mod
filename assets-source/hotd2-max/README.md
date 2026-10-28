# HOTD2 Max source assets

Recovered from the PC HOTD2 files in D:\games\House_of_the_dead_2_ISO. The identity is skeleton kind 0x02 and the znchain.bin geometry/texture bank. The actor has 16 nodes, 15 rigid node meshes, one pelvis attachment, and two integrated chainsaws (right-hand model 7123, left-hand model 7117).

The extracted texture bank has 29 descriptors, of which 23 are used by Max. Twelve decoded images contain non-opaque alpha. Max_alpha_audit.json records each alpha histogram and its material uses; Max_alpha_audit.png shows the masks. Materials with an opaque TSP pass ignore texture alpha. GoldSrc conversion thresholds active alpha into masked palette pixels and sets STUDIO_NF_MASKED in the compiled model.

The animation source combines zom.bin locomotion, pain, and death clips with nine Max-specific cha.bin clips (0xC1-0xC9). F13888_chain_saw_22.wav is the recovered chainsaw sample used by the contact event.

Files:

- source/: original compressed geometry, textures, motion banks, and WAV
- textures/: decoded RGBA PNGs
- Max_extracted.json: decoded meshes, rig, texture/material use, and clips
- Max_alpha_audit.json / Max_alpha_audit.png: alpha evidence
- goldsrc/: reproducible SMD/QC, compiled actor and body-piece MDLs, and material bitmaps
- conversion-report.json: converted skeleton, hitboxes, sequences, and installed file hashes
- source-sha256.json: original source file hashes
