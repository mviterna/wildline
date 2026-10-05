import os, sys, time, wave, json
from pathlib import Path
sys.path.insert(0, os.path.join(os.environ['TEMP'], 'codex-song-audio-tools'))
import numpy as np
import pyaudiowpatch as pa

out = Path(__file__).resolve().parent
with pa.PyAudio() as audio:
    device = audio.get_default_wasapi_loopback()
    rate = int(device['defaultSampleRate'])
    channels = min(2, device['maxInputChannels'])
    chunk = 1024
    with audio.open(format=pa.paInt16, channels=channels, rate=rate,
                    input=True, input_device_index=device['index'],
                    frames_per_buffer=chunk) as stream:
        print('RECORDING_READY: capturing speaker output for 85 seconds', flush=True)
        deadline = time.monotonic() + 85
        frames = []
        while time.monotonic() < deadline:
            data = stream.read(chunk, exception_on_overflow=False)
            frames.append(data)

pcm = np.frombuffer(b''.join(frames), dtype=np.int16).reshape(-1, channels)
with wave.open(str(out / 'capture-raw.wav'), 'wb') as wav:
    wav.setnchannels(channels)
    wav.setsampwidth(2)
    wav.setframerate(rate)
    wav.writeframes(pcm.tobytes())
print(json.dumps({'seconds':len(pcm)/rate,'sample_rate':rate,'channels':channels,
                  'peak':float(np.max(np.abs(pcm.astype(np.int32)))/32768),
                  'rms':float(np.sqrt(np.mean((pcm.astype(float)/32768)**2)))}), flush=True)
