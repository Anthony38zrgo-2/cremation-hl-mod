"""Smoke Featureful: arranca HLDS con el mod y verifica que sobrevive N segundos."""
import subprocess, sys, time
from pathlib import Path
seconds = int(sys.argv[1]) if len(sys.argv) > 1 else 12
ROOT = Path(__file__).resolve().parents[2]
log = ROOT / "Cremation" / "smoke-featureful.log"
with log.open("wb") as out:
    p = subprocess.Popen(
        [str(ROOT / "hlds.exe"), "-dev", "-console", "-game", "Cremation",
         "+developer", "1",
         "-insecure", "-nomaster", "-port", "27031", "+ip", "127.0.0.1",
         "+sv_lan", "1", "+maxplayers", "1", "+map", "rapidcore"],
        cwd=ROOT, stdin=subprocess.PIPE, stdout=out, stderr=subprocess.STDOUT)
    try:
        p.wait(timeout=seconds)
        print("FAIL: el servidor termino solo, codigo", p.returncode)
        sys.exit(1)
    except subprocess.TimeoutExpired:
        print("PASS: servidor vivo tras", seconds, "s con hl.dll Featureful")
    finally:
        if p.poll() is None:
            p.terminate()
        try:
            p.wait(timeout=10)
        except subprocess.TimeoutExpired:
            p.kill()
