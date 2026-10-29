"""Install Cremation's main-menu art, soundtrack, and direct-wave map entries."""
from pathlib import Path
import hashlib
import json
import math
import shutil
import struct

MOD = Path(__file__).resolve().parents[1]
SOURCE = MOD / 'assets-source/menu'
RESOURCE = MOD / 'resource'
BACKGROUND = RESOURCE / 'background'
TILE = 256


def _fit_image(path, size):
    from PIL import Image
    image = Image.open(path).convert('RGB')
    from PIL import ImageDraw, ImageFont, ImageFilter
    font_size = max(24, round(image.height * .09))
    font = ImageFont.truetype(str(RESOURCE / 'fonts' / 'Nosifer-Regular.ttf'), font_size)
    height = font_size * 3
    parts = []
    for label, skew, stroke in [('Half', True, 0), ('-', False, 0), ('DEATH', False, max(1, font_size // 60))]:
        part_width = math.ceil(font.getlength(label)) + font_size
        mask = Image.new('L', (part_width, height), 0)
        ImageDraw.Draw(mask).text((font_size // 3, font_size * 2), label, font=font,
                                  anchor='ls', fill=255, stroke_width=stroke, stroke_fill=255)
        if skew:
            shear = .22
            mask = mask.transform((part_width + math.ceil(height * shear), height), Image.Transform.AFFINE,
                                  (1, shear, -height * shear, 0, 1, 0), Image.Resampling.BICUBIC)
        bounds = mask.getbbox()
        parts.append(mask.crop((bounds[0], 0, bounds[2], height)))
    gap = max(8, font_size // 5)
    title = Image.new('L', (sum(part.width for part in parts) + gap * 2, height), 0)
    x = 0
    for part in parts:
        title.paste(part, (x, 0)); x += part.width + gap
    bounds = title.getbbox()
    title = title.crop(bounds)
    if title.width > image.width * .85:
        scale = image.width * .85 / title.width
        title = title.resize((round(title.width * scale), round(title.height * scale)), Image.Resampling.LANCZOS)
    x, y = (image.width - title.width) // 2, round(image.height * .045)
    outline = title.filter(ImageFilter.MaxFilter(7))
    image.paste((12, 0, 0), (x, y, x + title.width, y + title.height), outline)
    image.paste((220, 26, 35), (x, y, x + title.width, y + title.height), title)
    scale = min(size[0] / image.width, size[1] / image.height)
    image = image.resize((round(image.width * scale), round(image.height * scale)), Image.Resampling.LANCZOS)
    canvas = Image.new('RGB', size, (0, 0, 0))
    canvas.paste(image, ((size[0] - image.width) // 2, (size[1] - image.height) // 2))
    return canvas


def _background_layout(prefix, size):
    from PIL import Image
    width, height = size
    image = _fit_image(SOURCE / 'half_death_new.jpg', size)
    columns, rows = math.ceil(width / TILE), math.ceil(height / TILE)
    padded = Image.new('RGB', (columns * TILE, rows * TILE), (0, 0, 0))
    padded.paste(image, (0, 0))
    lines = [f'resolution\t{width}\t{height}', '']
    for row in range(rows):
        for column in range(columns):
            letter = chr(ord('a') + column)
            name = f'{prefix}_{row + 1}_{letter}_loading.tga'
            tile = padded.crop((column * TILE, row * TILE,
                                (column + 1) * TILE, (row + 1) * TILE))
            tile.save(BACKGROUND / name, format='TGA')
            lines.append(f'resource/background/{name}\tfit\t{column * TILE}\t{row * TILE}')
    return '\n'.join(lines) + '\n'


def install_background():
    BACKGROUND.mkdir(parents=True, exist_ok=True)
    (RESOURCE / 'BackgroundLayout.txt').write_text(
        _background_layout('cremation_menu_800', (800, 600)), encoding='ascii')
    (RESOURCE / 'HD_BackgroundLayout.txt').write_text(
        _background_layout('cremation_menu_hd', (1920, 1080)), encoding='ascii')


def _riff_chunk(name, payload):
    return name + struct.pack('<I', len(payload)) + payload + (b'\0' if len(payload) & 1 else b'')


def install_menu_music():
    import numpy as np
    import soundfile as sf
    source = SOURCE / 'introduction_attract_mode.flac'
    audio, rate = sf.read(source, dtype='int16', always_2d=True)
    if rate != 44100 or audio.shape[1] != 2 or len(audio) < rate * 8:
        raise ValueError('Menu soundtrack must be stereo 44.1 kHz and at least eight seconds long')

    crossfade = min(rate * 4, len(audio) // 8)
    fade = np.linspace(0.0, 1.0, crossfade, endpoint=True, dtype=np.float32)[:, None]
    tail = audio[-crossfade:].astype(np.float32)
    head = audio[:crossfade].astype(np.float32)
    blended = np.rint(tail * (1.0 - fade) + head * fade).clip(-32768, 32767).astype('<i2')
    loop_audio = np.concatenate((audio[:-crossfade].astype('<i2'), blended), axis=0)
    # The blended tail ends on the last frame of the head segment. Start the
    # repeating pass on that same sample so the loop seam is sample-identical.
    loop_start = crossfade - 1
    frames = len(loop_audio)

    fmt = struct.pack('<HHIIHH', 1, 2, rate, rate * 4, 4, 16)
    smpl_header = struct.pack('<9I', 0, 0, round(1_000_000_000 / rate), 60,
                              0, 0, 0, 1, 0)
    smpl_loop = struct.pack('<6I', 0, 0, loop_start, frames - 1, 0, 0)
    payload = b'WAVE' + _riff_chunk(b'fmt ', fmt) + _riff_chunk(b'smpl', smpl_header + smpl_loop)
    payload += _riff_chunk(b'data', loop_audio.tobytes())
    output = MOD / 'sound/music/menu_introduction.wav'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(b'RIFF' + struct.pack('<I', len(payload)) + payload)

    seam = np.abs(loop_audio[-1].astype(np.int32) - loop_audio[loop_start].astype(np.int32))
    manifest = {
        'source': str(source.relative_to(MOD)).replace('\\', '/'),
        'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'path': str(output.relative_to(MOD)).replace('\\', '/'),
        'rate': rate,
        'channels': 2,
        'frames': frames,
        'loop_start_frame': loop_start,
        'crossfade_seconds': crossfade / rate,
        'seam_delta_pcm': seam.tolist(),
    }
    (MOD / 'menu-music.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')


def install_wave_maps():
    source = MOD / 'maps/cs_mansion.bsp'
    if not source.is_file():
        raise FileNotFoundError(f'Build/install maps/cs_mansion.bsp before the wave menu: {source}')
    for wave in range(1, 5):
        shutil.copy2(source, MOD / f'maps/cs_mansion_wave{wave}.bsp')


def install():
    install_background()
    install_menu_music()
    install_wave_maps()
    print('Installed Cremation menu background, looping soundtrack, and direct-wave maps.')


if __name__ == '__main__':
    install()
