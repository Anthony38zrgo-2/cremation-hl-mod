"""Downsample the GLB's diffuse map to a GoldSrc-friendly 256x256 BMP."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "Cremation" / "assets-source" / "flak_cannon_model"
input_path = SOURCE / "flak_cannon_diffuse.png"
output_path = SOURCE / "flak_cannon.bmp"

with Image.open(input_path) as image:
    resized = image.convert("RGB").resize((256, 256), Image.Resampling.LANCZOS)
    indexed = resized.quantize(colors=256, method=Image.Quantize.MEDIANCUT)
    indexed.save(output_path, format="BMP")
print(f"Wrote {output_path} (256x256 8-bit BMP)")
