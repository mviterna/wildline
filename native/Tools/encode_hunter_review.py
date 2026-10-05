"""Encode rendered game frames using their simulation timestamps."""
import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('screenshots', type=Path)
parser.add_argument('log', type=Path)
parser.add_argument('output', type=Path)
parser.add_argument('--ffmpeg', type=Path, required=True)
parser.add_argument('--fps', type=float, default=30)
args = parser.parse_args()
files = {p.stem: p.resolve() for p in args.screenshots.rglob('*.png')}
frames = []
previous = -1
epoch = 0
for match in re.finditer(r'\[\s*(\d+)\]LogCore: Display: Tracing Screenshot "([^"]+)"', args.log.read_text(errors='replace')):
    tick, name = int(match[1]), match[2]
    if name not in files or not (name.startswith('Frame') or name.startswith('HunterCapture')):
        continue
    if tick < previous:
        epoch += 1000
    previous = tick
    frames.append((epoch + tick, files[name]))
if not frames:
    # Packaged builds omit editor screenshot tracing. The review requests one
    # numbered frame per fixed simulation tick, beginning at four seconds.
    frames = sorted((round(4 * args.fps) + int(name[5:]), path)
                    for name, path in files.items() if re.fullmatch(r'Frame\d+', name))
if len(frames) < 100:
    raise RuntimeError(f'Incomplete game recording: {len(frames)} frames')
args.output.parent.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='wildline-video-') as work:
    manifest = Path(work) / 'frames.txt'
    lines = []
    for i, (tick, path) in enumerate(frames):
        duration = (frames[i + 1][0] - tick) / args.fps if i + 1 < len(frames) else 1 / args.fps
        escaped = path.as_posix().replace("'", "'\\''")
        lines += [f"file '{escaped}'", f'duration {max(duration, 1 / args.fps):.9f}']
    lines.append(lines[-2])
    manifest.write_text('\n'.join(lines), encoding='utf-8')
    subprocess.run([str(args.ffmpeg), '-hide_banner', '-loglevel', 'warning', '-y',
                    '-f', 'concat', '-safe', '0', '-i', str(manifest), '-an',
                    '-c:v', 'libx264', '-preset', 'medium', '-crf', '19',
                    '-pix_fmt', 'yuv420p', '-fps_mode', 'cfr', '-r', str(args.fps),
                    '-movflags', '+faststart', str(args.output)], check=True)
for name, time in [('Walk', 6), ('Run', 9), ('Strafe', 12), ('Turn', 14.3),
                   ('Crouch', 18), ('Jump', 21.65), ('Land', 22.4),
                   ('Aim', 25.5), ('Pump', 28.85), ('Female', 32)]:
    _, source = min(frames, key=lambda frame: abs(frame[0] - time * args.fps))
    source = files.get('HunterCapture' + name, source)
    shutil.copyfile(source, args.output.parent / ('HunterCapture' + name + '.png'))
print(f'Encoded {len(frames)} game frames: {args.output}')
