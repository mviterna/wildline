import os,sys,json,subprocess,wave
from pathlib import Path
sys.path.insert(0,os.path.join(os.environ['TEMP'],'codex-song-analysis'))
sys.path.insert(0,os.path.join(os.environ['TEMP'],'codex-song-audio-tools'))
import numpy as np
from scipy.io import wavfile
import imageio_ffmpeg
out=Path(__file__).resolve().parent
report=json.loads((out/'mastering-report.json').read_text())
expected=report['instrumental_loop']['frames']
ffmpeg=imageio_ffmpeg.get_ffmpeg_exe()
results={}
decoded={}
for ext in ['wav','ogg','mp3']:
    path=out/f'Squirrel Scramble - instrumental loop.{ext}'
    p=subprocess.run([ffmpeg,'-hide_banner','-nostdin','-i',str(path),'-ar','48000',
                      '-ac','2','-f','f32le','-acodec','pcm_f32le','pipe:1'],capture_output=True)
    assert p.returncode==0
    x=np.frombuffer(p.stdout,np.float32).reshape(-1,2)
    assert len(x)==expected, f'{ext} has incorrect padding: {len(x)} versus {expected}'
    assert np.isfinite(x).all() and np.max(np.abs(x))<.9
    decoded[ext]=x
    results[ext]={'frames':len(x),'seconds':len(x)/48000,'peak':float(np.max(np.abs(x))),
                  'boundary_step':float(np.max(np.abs(x[-1]-x[0])))}
for ext in ['ogg','mp3']:
    results[ext]['correlation_with_wav']=float(np.corrcoef(decoded['wav'].ravel(),decoded[ext].ravel())[0,1])
    assert results[ext]['correlation_with_wav']>.98
(out/'loop-verification.json').write_text(json.dumps(results,indent=2))
print(json.dumps(results,indent=2))
