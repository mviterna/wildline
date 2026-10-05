import os, sys, json, wave
from pathlib import Path
sys.path.insert(0, os.path.join(os.environ['TEMP'], 'codex-song-analysis'))
os.environ['HF_HUB_DISABLE_SYMLINKS_WARNING'] = '1'
from faster_whisper import WhisperModel
import numpy as np
from scipy.signal import resample_poly
out = Path(__file__).resolve().parent
model = WhisperModel('base.en', device='cpu', compute_type='int8', cpu_threads=4,
                     download_root=os.path.join(os.environ['TEMP'],'codex-song-models'))
with wave.open(str(out/'Squirrel Scramble - recorded preview.wav')) as w:
    pcm=np.frombuffer(w.readframes(w.getnframes()),np.int16).reshape(-1,w.getnchannels())
    audio=resample_poly(pcm.astype(np.float32).mean(axis=1)/32768,1,3)
segments, _ = model.transcribe(audio,
                              language='en', beam_size=5, vad_filter=False,
                              condition_on_previous_text=False, word_timestamps=True)
result = []
for s in segments:
    item = {'start':s.start, 'end':s.end, 'text':s.text,
            'no_speech_probability':s.no_speech_prob,
            'words':[{'start':w.start,'end':w.end,'word':w.word,'probability':w.probability}
                     for w in (s.words or [])]}
    result.append(item)
    print(json.dumps(item),flush=True)
(out/'vocal-analysis.json').write_text(json.dumps(result,indent=2))
