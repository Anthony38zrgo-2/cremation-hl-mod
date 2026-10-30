"""Smoke de mapa arbitrario en modo dev (config errors a la vista)."""
import subprocess, sys
from pathlib import Path
seconds = int(sys.argv[2]) if len(sys.argv) > 2 else 20
gamemap = sys.argv[1] if len(sys.argv) > 1 else "test_wave1"
ROOT = Path(__file__).resolve().parents[2]
log = ROOT / "Cremation" / f"smoke-{gamemap}.log"
with log.open("wb") as out:
    p = subprocess.Popen(
        [str(ROOT / "hlds.exe"), "-dev", "-console", "-game", "Cremation",
         "+developer", "1",
         "-insecure", "-nomaster", "-port", "27032", "+ip", "127.0.0.1",
         "+sv_lan", "1", "+maxplayers", "1", "+map", gamemap],
        cwd=ROOT, stdin=subprocess.PIPE, stdout=out, stderr=subprocess.STDOUT)
    try:
        p.wait(timeout=seconds)
        print("FAIL: el servidor termino solo, codigo", p.returncode)
        sys.exit(1)
    except subprocess.TimeoutExpired:
        print(f"PASS: {gamemap} vivo tras {seconds} s")
    finally:
        if p.poll() is None:
            p.terminate()
        try:
            p.wait(timeout=10)
        except subprocess.TimeoutExpired:
            p.kill()
