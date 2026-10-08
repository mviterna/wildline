"""Encode the game's actual UI captures with its shot and menu audio.

Capture with -WildlineIntroReview -WildlineIntroFilm, then pass --frames,
--ffmpeg, --music and --out. This encodes captured frames without redrawing them.
"""
import argparse
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
for name in ('frames', 'ffmpeg', 'music', 'out'):
    parser.add_argument('--' + name, required=True)
args = parser.parse_args()
frames = Path(args.frames)
timing = json.loads((frames / 'Timing.json').read_text(encoding='utf-8-sig'))
if len(list(frames.glob('Frame*.png'))) != timing['frames']:
    raise RuntimeError(f"Expected exactly {timing['frames']} native capture frames")
duration = timing['frames'] / timing['fps']
shot_delay = round((timing['shotAt'] - 1 / timing['fps']) * 1000)
music_delay = round((timing['musicAt'] - 1 / timing['fps']) * 1000)
mix = (f'[1:a]volume=0.8,adelay={shot_delay}:all=1[shot];'
       f'[2:a]atrim=0:{duration},asetpts=PTS-STARTPTS,volume=0.455,'
       f'afade=t=in:st=0:d=0.4,adelay={music_delay}:all=1[music];'
       f'[shot][music]amix=inputs=2:normalize=0,apad,atrim=0:{duration}[a]')
subprocess.run([args.ffmpeg, '-hide_banner', '-nostdin', '-y',
                '-framerate', str(timing['fps']), '-start_number', '0',
                '-i', str(frames / 'Frame%04d.png'),
                '-i', str(frames / 'Shot.wav'), '-i', args.music,
                '-filter_complex', mix, '-map', '0:v', '-map', '[a]',
                '-t', str(duration), '-c:v', 'libx264', '-crf', '18', '-preset', 'medium',
                '-pix_fmt', 'yuv420p', '-c:a', 'aac', '-b:a', '192k',
                '-movflags', '+faststart', args.out], check=True)
