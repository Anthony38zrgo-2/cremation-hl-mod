"""Exercise Mansion perception, pursuit memory, door damage, melee and navigation in HLDS."""
import os, subprocess, time
from pathlib import Path
from validation_dll import using_validation_dll
MOD=Path(__file__).resolve().parents[1]
with using_validation_dll(MOD):
    diagnostics=MOD/'startup-diagnostics.txt'
    diagnostics.write_bytes(b'')
    with (MOD/'diagnostics/mansion-tactics-server.log').open('wb') as output:
        process=subprocess.Popen([str(MOD.parent/'hlds.exe'),'-console','-game','Cremation','-insecure','-nomaster','-port','27042','+sv_lan','1','+maxplayers','1','+map','cs_mansion'],cwd=MOD.parent,env=dict(os.environ,CREMATION_MANSION_TACTICS='1'),stdout=output,stderr=subprocess.STDOUT,stdin=subprocess.PIPE,creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline=time.monotonic()+65
            while time.monotonic()<deadline:
                text=diagnostics.read_text(errors='replace') if diagnostics.exists() else ''
                if 'MansionTacticsProbe PASS' in text or 'MansionTacticsProbe FAIL' in text:break
                if process.poll() is not None:break
                time.sleep(.25)
            print('\n'.join(line for line in text.splitlines() if 'MansionTacticsProbe' in line))
            assert 'MansionTacticsProbe PASS sight=1 memory=1 expired=1 door=1 closed=1 combat=1 auto=1 stairs=1 group=1 priority=1 bridge=1' in text, 'Inspect startup-diagnostics.txt and diagnostics/mansion-tactics-server.log'
            assert 'MansionNearRecovery PASS' in text, 'Recovery did not work with a nearby player'
            assert 'MansionBridgeObstacle PASS' in text, 'Blocked bridge pursuit did not follow its route'
            assert 'MansionVisionRange PASS' in text, 'Player beyond 120 units was detected'
            assert 'MansionCZRoomPolicy PASS' in text, 'CZ scientist and room-defense policy failed'
            assert 'MansionRunRange PASS' in text, 'Zombie run threshold failed'
        finally:
            if process.poll() is None:process.terminate()
            process.wait(timeout=5)
