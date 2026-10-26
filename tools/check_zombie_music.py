"""Native client playback/wrap and Blood precache integration smoke test."""
from pathlib import Path
import os,subprocess,time,json
ROOT=Path(__file__).resolve().parents[2];MOD=ROOT/'Cremation'
env=dict(os.environ,CREMATION_AUDIO_VALIDATE='1');env['PATH']=r'C:\Program Files (x86)\Steam'+os.pathsep+env['PATH']
(MOD/'startup-diagnostics.txt').write_text('');(MOD/'diagnostics/music-stream.log').write_text('')
with (MOD/'diagnostics/audio-client.log').open('wb') as log:
 p=subprocess.Popen([str(ROOT/'hl.exe'),'-game','Cremation','-num_edicts','2048','-window','-w','1280','-h','800','-console','+exec','cremation.cfg'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 try:
  deadline=time.monotonic()+30
  while time.monotonic()<deadline:
   assert p.poll() is None,('client exited',p.returncode)
   trace=(MOD/'startup-diagnostics.txt').read_text(errors='replace');music=(MOD/'diagnostics/music-stream.log').read_text(errors='replace')
   if 'Wrap 1' in music and 'Playback ' in music and 'RenderedFrames ' in music and 'ZombieVoice blood/zombie/' in trace:break
   time.sleep(.25)
  else:raise AssertionError((trace[-1000:],music))
  assert f"ZombieVoicePrecache {len(json.loads((MOD/'blood-zombie-sounds.json').read_text())['clips'])}" in trace
  assert not any(f'sfx_{i}.wav' in trace for i in json.loads((MOD/'zombie-sound-selection.json').read_text())['disabled_ids'])
  assert 'Failed' not in music and 'InvalidFormat' not in music,music
  (MOD/'diagnostics/audio-runtime.json').write_text(json.dumps(dict(music=music,zombie_voices=[s for s in trace.splitlines() if s.startswith('ZombieVoice')]),indent=2))
  print('PASS: real GoldSrc client, WinMM output opened, end-to-start loop crossed, native zombie events replaced with precached Blood clips.')
 finally:
  if p.poll() is None:p.terminate()
  p.wait(timeout=5)
