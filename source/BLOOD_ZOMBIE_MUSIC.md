# Blood zombies and myFuneral music

`tools/install_zombie_music.py` reads the local Blood `sounds.rff`, converting selected zombie SFX to mono PCM16/22050 Hz. The user kept 1100–1109 and deleted 1200–1207 in the review folder. Only the ten retained axe zombie clips remain active. `zombie-sound-selection.json` preserves that choice during regeneration; excluded game WAVs are archived under backups. The removed butcher attack/puke is replaced with the retained axe swipe. Event identification follows NBlood's `ai.cpp`, `actor.cpp`, `aizomba.cpp` and `aizombf.cpp`. `blood-zombie-sounds.json` records individual assets and hashes.

Only `monster_zombie` receives the replacements. Native idle, alert, pain, attack and claw events use the new sounds; appearance and footsteps are added, and the existing native Killed wrapper supplies death voices before gib/removal. Alternatives avoid immediate repeats; voice duration throttles overlap. Quake, fast, poison, cultists and the CZ zombie retain their own audio. AI, damage, status effects, gibbing and heart drops retain their existing logic.

The user MP3 stays unchanged. Two-pass loudness mastering targets -16 LUFS integrated, -1.5 dBTP maximum true peak and LRA 11. The MP3 export `sound/music/myfuneral_mastered.mp3` is stereo 44100 Hz/320 kbps (Media Foundation encoder). Measured decoded MP3 loudness is -16.03 LUFS and true peak is -1.50 dBTP; original loudness was -29.40 LUFS. The game uses PCM derived directly from the same mastered audio, with a six-second equal-power tail/head crossfade. Its `smpl` loop begins six seconds into the track, because those six seconds are already heard during the blend. The first playback retains the complete introduction. No further clipping adjustment was needed; the wrap difference is 31/78 PCM units for left/right. `music-loop.json` records provenance, mastering measurements and loop parameters.

At the start of gameplay, `666_intro_myfuneral.wav` first plays the mastered
`666` track and blends its final six seconds into myFuneral. The embedded loop
returns to myFuneral's original loop point, so the 666 intro does not repeat.
After Judgment music interrupts the intro or OST, playback returns to the
ordinary myFuneral loop. `tools/install_666_intro.py` rebuilds the master and
combined track; `666-intro-mastering.json` records its loudness and loop points.

The client streams four WinMM buffers continuously, wrapping inside buffer fills. Software volume follows the engine master `volume`, with an additional `cremation_music_volume` multiplier (default 0.65, increased from 0.5; current user config also updated). `cremation_music 0` disables music; `cremation_music 1` enables it. Gain changes are smoothed. Disconnect/HUD shutdown closes and joins the audio worker. Both direct exports and the native `F` table are hooked.

`tools/check_zombie_music.py` uses a real GoldSrc client, verifies sound precache/native voice events, actual completed output buffers, a loop crossing, and clean audio-device closure on disconnect. Only under `CREMATION_AUDIO_VALIDATE` does playback start half a second before EOF and disconnect automatically after eight seconds, to exercise the loop and cleanup quickly. Normal sessions start at the beginning.
