"""Master the original capture, and render a circular instrumental loop."""
import os, sys, wave, json, re, subprocess, base64
from pathlib import Path
sys.path.insert(0, os.path.join(os.environ['TEMP'],'codex-song-analysis'))
sys.path.insert(0, os.path.join(os.environ['TEMP'],'codex-song-audio-tools'))
import numpy as np
from scipy.io import wavfile
from scipy.interpolate import CubicSpline
import imageio_ffmpeg

out=Path(__file__).resolve().parent
ffmpeg=imageio_ffmpeg.get_ffmpeg_exe()
def run(args):
    p=subprocess.run([ffmpeg,'-hide_banner','-nostdin','-y',*args],capture_output=True,text=True)
    if p.returncode: raise RuntimeError(p.stderr[-4000:])
    return p
def loudness(path):
    p=run(['-i',str(path),'-af','loudnorm=I=-16:TP=-1.5:LRA=11:print_format=json','-f','null','-'])
    return json.loads(re.findall(r'\{\s*"input_i".*?\}',p.stderr,re.S)[-1])

with wave.open(str(out/'Squirrel Scramble - recorded preview.wav')) as w:
    sr=w.getframerate(); nch=w.getnchannels()
    original=np.frombuffer(w.readframes(w.getnframes()),np.int16).reshape(-1,nch)
x=original.astype(np.float64)/32768
fixed=0
for ch in range(nch):
    clipped=(original[:,ch]==32767)|(original[:,ch]==-32768)
    ids=np.flatnonzero(clipped)
    groups=np.split(ids,np.flatnonzero(np.diff(ids)>1)+1)
    for group in groups:
        if not len(group): continue
        lo=max(0,int(group[0])-6); hi=min(len(x),int(group[-1])+7)
        good=np.arange(lo,hi)[~clipped[lo:hi]]
        if len(good)<4 or good[0]>=group[0] or good[-1]<=group[-1]: continue
        estimate=CubicSpline(good,x[good,ch])(group)
        # Restore only peaks for which the interpolation has the same sign
        # and exceeds the clipped value. Limit extrapolated peak height.
        old=x[group,ch]
        valid=(np.sign(estimate)==np.sign(old))&(np.abs(estimate)>np.abs(old))
        x[group[valid],ch]=np.clip(estimate[valid],-1.06,1.06)
        fixed+=int(valid.sum())

working=out/'master-work.wav'
wavfile.write(working,sr,x.astype(np.float32))
balanced=out/'balanced-work.wav'
filters='highpass=f=28,equalizer=f=180:t=q:w=0.75:g=-0.7,equalizer=f=3200:t=q:w=0.8:g=-0.4'
run(['-i',str(working),'-af',filters,'-c:a','pcm_f32le',str(balanced)])
_,x=wavfile.read(balanced); x=x.astype(np.float64)

analysis=json.loads((out/'loop-analysis.json').read_text())
candidate=analysis['candidates'][0]
start=int(round(candidate['start']*sr)); length=int(round(candidate['duration']*sr))
end=start+length
crossfade=int(round(analysis['beat_seconds']*.5*sr))
head=x[start:start+crossfade]
tail=x[end:end+crossfade]
theta=np.linspace(0,np.pi/2,crossfade)[:,None]
wa=np.cos(theta); wb=np.sin(theta)
corr=float(np.sum(head*tail)/np.sqrt(np.sum(head**2)*np.sum(tail**2)))
blend=(tail*wa+head*wb)/np.sqrt(1+2*np.clip(corr,-.5,.5)*wa*wb)
# The seam continues directly from source end-1 to source end. The blend
# then moves into the start phrase; the central audio stays untouched.
loop=np.concatenate([blend,x[start+crossfade:end]],axis=0)
assert len(loop)==length

def export(audio,name,looping=False):
    temp=out/(name+'-work.wav')
    wavfile.write(temp,sr,audio.astype(np.float32))
    measured=loudness(temp)
    gain_db=-16-float(measured['input_i'])
    gain_db=min(gain_db,-1.5-float(measured['input_tp']))
    final=audio*10**(gain_db/20)
    wavfile.write(temp,sr,final.astype(np.float32))
    title='Squirrel Scramble - '+name
    paths={}
    for ext,codec in [('wav',['-c:a','pcm_s24le']),('mp3',['-c:a','libmp3lame','-b:a','320k'])]:
        target=out/(title+'.'+ext)
        run(['-i',str(temp),*codec,'-metadata','title='+title,str(target)])
        paths[ext]=str(target)
    if looping:
        target=out/(title+'.ogg')
        run(['-i',str(temp),'-c:a','libvorbis','-q:a','9',
             '-metadata','title='+title,'-metadata','LOOPSTART=0',
             '-metadata','LOOPEND='+str(len(final)),'-metadata','LOOPLENGTH='+str(len(final)),str(target)])
        paths['ogg']=str(target)
        # Compact PCM16 embedding is generated from the float master once,
        # rather than decoding a lossy file for the infinite-loop player.
        player_audio=out/'player-audio-work.wav'
        run(['-i',str(temp),'-c:a','pcm_s16le',str(player_audio)])
    for path in paths.values(): run(['-i',path,'-f','null','-'])
    verified=loudness(Path(paths['wav']))
    temp.unlink()
    return final,paths,verified

full=x.copy(); n=int(sr*.008)
full[:n]*=np.linspace(0,1,n)[:,None]
n=int(sr*.08); full[-n:]*=np.linspace(1,0,n)[:,None]
full,full_paths,full_stats=export(full,'polished song')
loop,loop_paths,loop_stats=export(loop,'instrumental loop',True)

# A three-cycle render is a review aid; each boundary uses exactly the
# same samples as continuous playback of the looping buffer.
preview=out/'loop-review-work.wav'
wavfile.write(preview,sr,np.tile(loop,(3,1)).astype(np.float32))
run(['-i',str(preview),'-c:a','libmp3lame','-b:a','320k',
     str(out/'Squirrel Scramble - loop review 3 cycles.mp3')])
preview.unlink()
delta=np.diff(np.concatenate([loop[-sr:],loop[:sr]]),axis=0)
seam_step=float(np.max(np.abs(loop[0]-loop[-1])))
typical_step=float(np.percentile(np.max(np.abs(delta),axis=1),99))
assert seam_step < max(.002,typical_step*1.2), 'Loop seam is discontinuous'
assert abs(float(loop_stats['input_i'])+16)<.15, 'Loop loudness is off target'
assert float(loop_stats['input_tp']) < -1.45, 'Loop clips at its true peak'

vocal=json.loads((out/'vocal-analysis.json').read_text())
confident_words=[w for segment in vocal for w in segment['words'] if w['probability']>.45]
in_loop=[w for w in confident_words if w['start']>=start/sr and w['start']<(end+crossfade)/sr]
assert not in_loop, 'Detected a confident vocal in the selected loop'
report={'source':'Original lossless playback recording','sample_rate':sr,'channels':nch,
        'restored_clipped_samples':fixed,'filters':filters,
        'full_song':{'duration_seconds':len(full)/sr,'loudness_lufs':full_stats['input_i'],
                     'true_peak_db':full_stats['input_tp'],'files':full_paths},
        'instrumental_loop':{'source_start_seconds':start/sr,'source_end_seconds':end/sr,
            'duration_seconds':len(loop)/sr,'frames':len(loop),'beats':candidate['beats'],
            'estimated_bpm':analysis['estimated_bpm'],'crossfade_seconds':crossfade/sr,
            'loudness_lufs':loop_stats['input_i'],'true_peak_db':loop_stats['input_tp'],
            'boundary_step':seam_step,'nearby_99th_percentile_step':typical_step,
            'confident_vocals_detected':len(in_loop),'files':loop_paths},
        'last_confident_vocal_end_seconds':max(w['end'] for w in confident_words)}
(out/'mastering-report.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))

data=base64.b64encode((out/'player-audio-work.wav').read_bytes()).decode()
template=(out/'loop-player-template.html').read_text(encoding='utf-8')
template=template.replace('__AUDIO_BASE64__',data).replace('__DURATION__',f'{len(loop)/sr:.2f}')
(out/'Loop Player.html').write_text(template,encoding='utf-8')
for path in [working,balanced,out/'player-audio-work.wav']:
    path.unlink(missing_ok=True)
