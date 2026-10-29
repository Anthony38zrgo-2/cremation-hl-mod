"""Master the 666 intro and join it to the existing gameplay soundtrack."""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess
import wave

import numpy as np

MOD = Path(__file__).resolve().parents[1]
FFMPEG = Path('C:/Users/bill/AppData/Local/CapCut/Apps/9.3.0.3970/ffmpeg.exe')
SOURCE = MOD / 'assets-source/music/666_intro_source.wav'
MASTER = MOD / 'sound/music/666_intro_mastered.wav'
OST = MOD / 'sound/music/myfuneral_loop.wav'
OUTPUT = MOD / 'sound/music/666_intro_myfuneral.wav'
RATE = 44100
FADE = 6 * RATE


def loudness(path):
    result = subprocess.run([str(FFMPEG), '-hide_banner', '-nostats', '-i', str(path),
                             '-af', 'loudnorm=I=-16:TP=-1.5:LRA=11:print_format=json',
                             '-f', 'null', '-'], capture_output=True, text=True, check=True)
    return json.loads(re.findall(r'\{[^{}]*"input_i"[^{}]*\}', result.stderr, re.S)[-1])


def read_pcm(path):
    with wave.open(str(path), 'rb') as wav:
        if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) != (2, 2, RATE):
            raise ValueError(f'Expected stereo 16-bit 44100 Hz PCM: {path}')
        return np.frombuffer(wav.readframes(wav.getnframes()), dtype='<i2').reshape(-1, 2).copy()


def loop_start(path):
    with path.open('rb') as f:
        f.seek(12)
        while header := f.read(8):
            chunk, size = struct.unpack('<4sI', header)
            if chunk == b'smpl' and size >= 60:
                values = struct.unpack('<15I', f.read(60))
                return values[11]
            f.seek(size + (size & 1), 1)
    raise ValueError(f'No smpl loop in {path}')


def main():
    initial = loudness(SOURCE)
    normalizer = ('loudnorm=I=-16:TP=-1.5:LRA=11:measured_I={input_i}:'
                  'measured_TP={input_tp}:measured_LRA={input_lra}:'
                  'measured_thresh={input_thresh}:offset={target_offset}:linear=true').format(**initial)
    subprocess.run([str(FFMPEG), '-v', 'error', '-y', '-i', str(SOURCE), '-af', normalizer,
                    '-ar', str(RATE), '-ac', '2', '-c:a', 'pcm_s16le', str(MASTER)], check=True)
    final = loudness(MASTER)
    intro, ost = read_pcm(MASTER), read_pcm(OST)
    original_loop = loop_start(OST)
    if min(len(intro), len(ost)) <= FADE or not 0 <= original_loop < len(ost):
        raise ValueError('Track too short or invalid OST loop')
    # A linear crossfade keeps the sum bounded even for correlated material.
    fade = np.linspace(0, 1, FADE, dtype=np.float32)[:, None]
    blend = np.rint(intro[-FADE:].astype(np.float32) * (1-fade) + ost[:FADE].astype(np.float32) * fade).clip(-32768, 32767).astype('<i2')
    loop_frame = len(intro) - FADE + original_loop
    with wave.open(str(OUTPUT), 'wb') as wav:
        wav.setparams((2, 2, RATE, 0, 'NONE', 'not compressed'))
        for part in (intro[:-FADE], blend, ost[FADE:]):
            wav.writeframes(part.tobytes())
    # Preserve the existing OST wrap point after its first complete playback.
    size = OUTPUT.stat().st_size
    smpl = struct.pack('<15I', 0, 0, round(1e9 / RATE), 60, 0, 0, 0, 1, 0,
                       0, 0, loop_frame, len(intro) + len(ost) - FADE - 1, 0, 0)
    with OUTPUT.open('r+b') as f:
        f.seek(0, 2); f.write(b'smpl' + struct.pack('<I', len(smpl)) + smpl)
        f.seek(4); f.write(struct.pack('<I', size + 8 + len(smpl) - 8))
    report = dict(source='assets-source/music/666_intro_source.wav',
                  source_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                  original_loudness=initial, mastered_loudness=final,
                  target={'integrated_lufs': -16, 'true_peak_dbtp': -1.5, 'lra': 11},
                  intro_seconds=len(intro)/RATE, crossfade_seconds=FADE/RATE,
                  ost='sound/music/myfuneral_loop.wav', ost_loop_start=original_loop,
                  combined='sound/music/666_intro_myfuneral.wav', combined_loop_start=loop_frame)
    (MOD/'666-intro-mastering.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
