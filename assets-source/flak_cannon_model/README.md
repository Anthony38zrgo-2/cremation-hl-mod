# Flak Cannon viewmodel source

This source replaces the weapon mesh in `models/cremation/v_bounce_shotgun.mdl`.
It retains the donor shotgun's hand geometry, skeleton, shell bodygroup, ten
animation sequences, and animation events. Only the old shotgun triangles are
removed; the GLB mesh is rigidly attached to `Bip01 R Hand`.

The Flak Cannon mesh is uniformly scaled at 1.1 and centered on the original
shotgun mesh bounds (32.0 units long versus 34.8 units for the shotgun mesh).
Its long axis is rotated to follow the original muzzle direction. This keeps
the GLB proportions while bringing its overall size close to the model already
used by the weapon. Its diffuse texture is reduced to a 256x256 BMP for the
GoldSrc model compiler.

To regenerate and install the viewmodel from `flak_cannon.glb`, run this from
the Half-Life folder:

```powershell
.\Cremation\tools\compile_flak_cannon.ps1
```

The script uses Blender 5.2, Python with Pillow, and the SDK `studiomdl.exe`.
The QC compiles to `Cremation/models/cremation/v_bounce_shotgun.mdl`; the
compiled MDL is also copied into this folder as the installer source.
