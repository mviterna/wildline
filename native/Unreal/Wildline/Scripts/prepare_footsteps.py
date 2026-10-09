"""Keep the four original footfalls; trim gaps and apply short click-free fades."""
from pathlib import Path
import array
import json
import wave

root = Path(__file__).resolve().parents[1] / 'ArtSource/Audio'
with wave.open(str(root / 'FootstepsReference.wav'), 'rb') as source:
    assert source.getnchannels() == 1 and source.getsampwidth() == 2
    rate = source.getframerate()
    pcm = array.array('h', source.readframes(source.getnframes()))
segments = [(0.426, 0.735), (0.900, 1.167), (1.267, 1.611), (1.633, 1.920)]
for index, (start, end) in enumerate(segments, 1):
    step = pcm[round(start * rate):round(end * rate)]
    for i in range(len(step)):
        fade = min(1, i / (rate * .002), (len(step) - 1 - i) / (rate * .012))
        step[i] = round(step[i] * fade)
    with wave.open(str(root / f'Footstep{index:02}.wav'), 'wb') as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(rate)
        output.writeframes(step.tobytes())
    print(f'FOOTSTEP {index}: {len(step) / rate:.3f}s, peak {max(map(abs, step))}')
(root / 'footstep-segments.json').write_text(json.dumps({'source': 'https://www.youtube.com/watch?v=OY1LwHQZF3s', 'segments_seconds': segments, 'sample_rate': rate, 'run_playback_rate': 1.1}, indent=2))
