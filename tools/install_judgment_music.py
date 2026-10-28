"""Prepare the supplied Judgment track for Cremation's existing PCM streamer."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import wave

MOD = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, default=Path(r'C:\Users\bill\Downloads\Death - Leprosy (Instrumental Cover With Oscar Padilla).mp3'))
    args = parser.parse_args()
    ffmpeg = shutil.which('ffmpeg') or r'C:\Users\bill\AppData\Local\CapCut\Apps\9.3.0.3970\ffmpeg.exe'
    if not args.source.is_file():
        raise FileNotFoundError(args.source)
    output = MOD/'sound/music/judgment_leprosy.wav'
    output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([ffmpeg, '-v', 'error', '-y', '-i', str(args.source), '-vn',
                    '-ar', '44100', '-ac', '2', '-c:a', 'pcm_s16le', str(output)], check=True)
    with wave.open(str(output), 'rb') as audio:
        assert audio.getframerate() == 44100 and audio.getnchannels() == 2 and audio.getsampwidth() == 2
        duration = audio.getnframes()/audio.getframerate()
        assert duration > 30
    report = dict(source=str(args.source), source_sha256=hashlib.sha256(args.source.read_bytes()).hexdigest(),
                  installed='sound/music/judgment_leprosy.wav',
                  installed_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),
                  duration_seconds=duration, rate=44100, channels=2, pcm_bits=16,
                  trigger='Judgment encounter active; returns to normal music after defeat')
    (MOD/'assets-source/hotd2-judgment/music.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
