"""Convert the retained mastered loop to Unreal's lossless PCM16 import format."""
from pathlib import Path
import array
import struct
import wave

project = Path(__file__).resolve().parents[1]
repository = project.parents[2]
source = repository / 'audio-exports/Squirrel Scramble - instrumental loop.wav'
destination = project / 'ArtSource/Presentation/MenuBluegrass.wav'
data = source.read_bytes()
assert data[:4] == b'RIFF' and data[8:12] == b'WAVE'
chunks = {}
offset = 12
while offset + 8 <= len(data):
    name, length = struct.unpack_from('<4sI', data, offset)
    chunks[name] = data[offset + 8:offset + 8 + length]
    offset += 8 + length + (length & 1)
fmt = chunks[b'fmt ']
tag, channels, rate, _, _, bits = struct.unpack_from('<HHIIHH', fmt)
if tag == 65534:
    tag = struct.unpack_from('<H', fmt, 24)[0]
if tag == 3 and bits == 32:
    floating = array.array('f', chunks[b'data'])
    pcm = array.array('h', (round(max(-1, min(1, x)) * 32767) for x in floating))
elif tag == 1 and bits == 16:
    pcm = array.array('h', chunks[b'data'])
elif tag == 1 and bits == 24:
    raw = chunks[b'data']
    pcm = array.array('h', (int.from_bytes(raw[i:i + 3], 'little', signed=True) >> 8 for i in range(0, len(raw), 3)))
else:
    raise ValueError(f'Unsupported WAV format {tag}, {bits} bits')
destination.parent.mkdir(parents=True, exist_ok=True)
with wave.open(str(destination), 'wb') as output:
    output.setnchannels(channels)
    output.setsampwidth(2)
    output.setframerate(rate)
    output.writeframes(pcm.tobytes())
print(f'Presentation loop: {len(pcm) / channels / rate:.6f}s, {rate} Hz, {channels} channels, PCM16')
