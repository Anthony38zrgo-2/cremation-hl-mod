"""Extract Judgment's original geometry, textures, and motion from HOTD2 DATA.CAB."""
from __future__ import annotations

import subprocess
from pathlib import Path

ISO = Path(r"D:\games\House_of_the_dead_2_ISO")
OUT = Path(__file__).resolve().parents[1] / "assets-source" / "hotd2-judgment" / "source"
FILES = (
    "F12629_boss1.bin",
    "F12705_boss1q.bin", "F12706_boss1z.bin", "F12707_boss1z_wing.bin",
    "F13033_pol_boss1q.bin", "F13034_pol_boss1z.bin", "F13035_pol_boss1z_wing.bin",
    "F13411_boss1q.bin", "F13412_boss1z.bin", "F13413_boss1z_wing.bin",
)


def main() -> None:
    cab = ISO / "DATA.CAB"
    if not cab.is_file():
        raise FileNotFoundError(cab)
    OUT.mkdir(parents=True, exist_ok=True)
    for filename in FILES:
        dest = OUT / filename
        if dest.exists() and dest.stat().st_size:
            continue
        result = subprocess.run(
            ["expand.exe", str(cab), f"-F:{filename}", str(OUT)],
            capture_output=True, text=True, check=False,
        )
        if result.returncode or not dest.is_file():
            raise RuntimeError(f"Could not extract {filename}: {result.stdout}\n{result.stderr}")
    print(f"Extracted {len(FILES)} Judgment resources to {OUT}")


if __name__ == "__main__":
    main()
