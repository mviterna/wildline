import wave, json
from pathlib import Path
import numpy as np
out = Path(__file__).resolve().parent
with wave.open(str(out/'Squirrel Scramble - recorded preview.wav')) as w:
    rate=w.getframerate()
    audio=np.frombuffer(w.readframes(w.getnframes()),np.int16).reshape(-1,2).mean(axis=1)/32768
x=audio[::4]; sr=rate//4; hop=120
def stft(n):
    frames=np.lib.stride_tricks.sliding_window_view(x,n)[::hop]
    return np.abs(np.fft.rfft(frames*np.hanning(n),axis=1)), (np.arange(len(frames))*hop+n/2)/sr
spec,times=stft(1024)
bins=np.fft.rfftfreq(1024,1/sr)
logged=np.log1p(spec[:,(bins>100)&(bins<5000)]*10)
flux=np.maximum(np.diff(logged,axis=0),0).mean(axis=1)
onsets=np.pad(flux,(1,0)); onsets=(onsets-onsets.mean())/(onsets.std()+1e-10)
z=onsets[(times>12)&(times<54)]
ac=np.correlate(z,z,'full')[len(z)-1:]
lags=np.arange(36,51)
scores=ac[lags]+.7*ac[2*lags]+.4*ac[4*lags]
lag=int(lags[np.argmax(scores)])
periods=np.linspace(max(.35,lag*.01-.012),min(.51,lag*.01+.012),1001)
best=None
for period in periods:
    # Compare each beat with its neighbours several measures away to refine
    # the grid beyond the STFT frame interval.
    score=sum(np.interp(period*k/.01,np.arange(len(ac)),ac)*weight
              for k,weight in [(1,1),(2,.7),(4,.6),(8,.5),(16,.35),(32,.2)])
    if best is None or score>best[0]: best=(float(score),float(period))
period=best[1]
phase_scores=[]
for phase in np.linspace(0,period,300,endpoint=False):
    beats=np.arange(phase,55,period); beats=beats[(beats>12)&(beats<54)]
    phase_scores.append((float(np.interp(beats,times,onsets).mean()),float(phase)))
phase=max(phase_scores)[1]
big,tbig=stft(4096)
freq=np.fft.rfftfreq(4096,1/sr)
valid=(freq>65)&(freq<2800)
notes=np.rint(69+12*np.log2(freq[valid]/440)).astype(int)%12
chroma=np.zeros((len(big),12))
for pc in range(12):
    chroma[:,pc]=(big[:,valid][:,notes==pc]**2).sum(axis=1)
chroma=np.sqrt(chroma); chroma/=np.linalg.norm(chroma,axis=1,keepdims=True)+1e-10
beats=np.arange(phase,60,period)
candidates=[]
for nbeats in [64,80,96]:
    for start in beats[(beats>=14)&(beats<24)]:
        end=start+nbeats*period
        if end+2*period>55: continue
        a=chroma[(tbig>=start)&(tbig<start+4*period)].mean(axis=0)
        b=chroma[(tbig>=end)&(tbig<end+4*period)].mean(axis=0)
        cosine=float(np.dot(a,b)/(np.linalg.norm(a)*np.linalg.norm(b)))
        edge=float(np.interp(start,times,onsets)+np.interp(end,times,onsets))
        strength=float(cosine + .008*edge + .00018*nbeats)
        candidates.append({'start':float(start),'end':float(end),'beats':nbeats,
                           'duration':float(end-start),'chroma_similarity':cosine,
                           'strength':strength})
candidates.sort(key=lambda c:c['strength'],reverse=True)
report={'estimated_bpm':60/period,'beat_seconds':period,'beat_phase':phase,
        'candidates':candidates[:20]}
(out/'loop-analysis.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
