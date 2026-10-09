# Footstep audio

Requested source: [Concrete Footsteps (Counter Strike Source) - Sound Effects for editing](https://www.youtube.com/watch?v=OY1LwHQZF3s), uploaded by Sound Library.
Original game audio credited to Valve / Counter-Strike: Source. This records
the source attribution; the uploader does not identify a specific license.

Retained the downloaded reference and mono PCM conversion. Four individual
footfalls are trimmed from the clip with short edge fades, without changing
their normal pitch or normalizing away the original variation. Walking plays
these at 1.0x; running plays the same samples at 1.1x. Footfalls are triggered
by grounded movement and animation rather than a continuously playing loop.

`prepare_footsteps.py` records the trims in `footstep-segments.json`;
`import_footsteps.py` imports the four waves under `/Game/Wildline/Audio`.
