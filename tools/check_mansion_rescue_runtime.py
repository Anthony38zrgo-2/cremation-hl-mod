"""Exercise native scientist creation, same-entity relocation and defeat in GoldSrc."""
import os, subprocess, time
from pathlib import Path
from validation_dll import using_validation_dll
MOD=Path(__file__).resolve().parents[1]
with using_validation_dll(MOD):
    diagnostics=MOD/'startup-diagnostics.txt'
    diagnostics.write_bytes(b'')
    offset=0
    with (MOD/'diagnostics/scientist-event-server.log').open('wb') as output:
        process=subprocess.Popen([str(MOD.parent/'hlds.exe'),'-console','-game','Cremation','-insecure','-nomaster','-port','27039','+sv_lan','1','+maxplayers','1','+map','cs_mansion_wave1'],cwd=MOD.parent,env=dict(os.environ,CREMATION_RESCUE_VALIDATE='1'),stdout=output,stderr=subprocess.STDOUT,stdin=subprocess.PIPE,creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline=time.monotonic()+30
            while time.monotonic()<deadline:
                text=diagnostics.read_bytes()[offset:].decode(errors='replace') if diagnostics.exists() else ''
                if 'MansionRescueProbe' in text:break
                time.sleep(.25)
            print(text[-2500:])
            server_log=(MOD/'diagnostics/scientist-event-server.log').read_text(errors='replace')
            assert 'Message name too long' not in server_log
            assert "couldn't exec maps/cs_mansion_wave1_load.cfg" not in server_log
            assert 'MansionRescueProbe PASS native_private=1' in text, 'Native rescue fixture failed; inspect diagnostics/scientist-event-server.log'
            assert 'same_entity=1 raiders=2 ground=3 defeat=1 paths=2' in text
            variant=os.environ.get('CREMATION_SCIENTIST_VARIANT')
            if variant in ('0','2'):
                assert f'variant={variant}' in text
                assert f'MansionScientistPain {"female" if variant == "2" else "male"}' in text
        finally:
            if process.poll() is None:process.terminate()
            process.wait(timeout=5)
