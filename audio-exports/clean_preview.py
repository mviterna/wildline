"""Trim the captured preview and produce a restrained, normalized MP3 master."""
import os, sys, wave, json, re, subprocess
from pathlib import Path
import numpy as np
sys.path.insert(0, os.path.join(os.environ['TEMP'], 'codex-song-audio-tools'))
import imageio_ffmpeg
from mutagen.mp3 import MP3

out = Path(__file__).resolve().parent
ffmpeg = imageio_ffmpeg.get_ffmpeg_exe()
with wave.open(str(out / 'capture-raw.wav')) as wav:
    rate = wav.getframerate()
    channels = wav.getnchannels()
    pcm = np.frombuffer(wav.readframes(wav.getnframes()), np.int16).reshape(-1, channels)

# Ignore the steady low-level noise before playback. The preview rises well
# above this measured noise floor and then lasts exactly sixty seconds.
block = rate // 100
blocks = pcm[:len(pcm)//block*block].astype(np.float32).reshape(-1, block, channels)/32768
rms = np.sqrt(np.mean(blocks**2, axis=(1,2)))
baseline = float(np.median(rms[:5*100]))
threshold = max(.005, baseline*12)
start_block = None
for i in range(len(rms)-100):
    if rms[i] > threshold and np.count_nonzero(rms[i:i+100] > threshold) > 90:
        start_block = i
        break
if start_block is None:
    raise RuntimeError('No sustained song playback found')
start = max(0, start_block * block - int(.03*rate))
segment = pcm[start:start+60*rate]
if len(segment) != 60*rate:
    raise RuntimeError('The recording does not contain the complete preview')
if float(np.sqrt(np.mean((segment[-2*rate:].astype(np.float32)/32768)**2))) < .005:
    raise RuntimeError('The detected preview ends prematurely')

trimmed = out / 'Squirrel Scramble - recorded preview.wav'
with wave.open(str(trimmed), 'wb') as wav:
    wav.setnchannels(channels)
    wav.setsampwidth(2)
    wav.setframerate(rate)
    wav.writeframes(segment.tobytes())

clip_fraction = float(np.mean(np.abs(segment.astype(np.int32)) >= 32760))
base_filter = 'highpass=f=30,afade=t=in:st=0:d=0.008,afade=t=out:st=59.92:d=0.08'
if clip_fraction > .0005:
    base_filter = 'adeclip=w=55:o=75:a=12:t=10:n=1000,' + base_filter

def run(args):
    result = subprocess.run([ffmpeg, '-hide_banner', '-nostdin', '-y', *args],
                            capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stderr[-4000:])
    return result

measurement = run(['-i', str(trimmed), '-af', base_filter + ',loudnorm=I=-16:TP=-1.5:LRA=11:print_format=json', '-f', 'null', '-'])
matches = re.findall(r'\{\s*"input_i".*?\}', measurement.stderr, re.S)
if not matches:
    raise RuntimeError('Loudness measurement failed')
measured = json.loads(matches[-1])
normalization = (f'loudnorm=I=-16:TP=-1.5:LRA=11:measured_I={measured["input_i"]}'
                 f':measured_TP={measured["input_tp"]}:measured_LRA={measured["input_lra"]}'
                 f':measured_thresh={measured["input_thresh"]}:offset={measured["target_offset"]}'
                 ':linear=true:print_format=json')
cleaned = out / 'Squirrel Scramble - cleaned preview.mp3'
result = run(['-i',str(trimmed),'-af',base_filter+','+normalization,
              '-ar','48000','-c:a','libmp3lame','-b:a','320k',
              '-metadata','title=Squirrel Scramble Game - Cleaned Preview',
              '-metadata','comment=Recorded from preview playback; trimmed and normalized locally.',
              str(cleaned)])
run(['-i',str(trimmed),'-c:a','libmp3lame','-b:a','320k',
     '-metadata','title=Squirrel Scramble Game - Recorded Preview',
     str(out / 'Squirrel Scramble - recorded preview.mp3')])
run(['-i',str(cleaned),'-f','null','-'])
check = run(['-i',str(cleaned),'-af','loudnorm=I=-16:TP=-1.5:LRA=11:print_format=json','-f','null','-'])
checked = json.loads(re.findall(r'\{\s*"input_i".*?\}',check.stderr,re.S)[-1])
info = MP3(cleaned).info
report = {'captured_preview_start_seconds':start/rate,'trimmed_pcm_seconds':60,
          'recording_noise_floor_rms':baseline,'clipped_sample_fraction_before_cleanup':clip_fraction,
          'input_loudness_lufs':measured['input_i'],'cleaned_loudness_lufs':checked['input_i'],
          'cleaned_true_peak_db':checked['input_tp'],'mp3_duration_seconds':info.length,
          'mp3_bitrate':info.bitrate,'sample_rate':info.sample_rate,'channels':info.channels,
          'filters':base_filter,'cleaned_file':str(cleaned)}
(out/'cleanup-report.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
