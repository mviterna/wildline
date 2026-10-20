# Startup presentation

2026-10-19 update: the active soundtrack is `MenuRap.wav`, a complete 140-second
48 kHz stereo PCM16 decode of the supplied ElevenLabs MP3, also retained as
`MenuRapOriginal.mp3`. The game uses `SW_MenuRap`; all previous music source and
`SW_MenuBluegrass` remain in the repository. The existing music fade and saved
volume behavior are unchanged. The track loops while the intro/menu is audible.

The updated `StudioLogo.png` adds the supplied dinosaur on the left and bear on
the right, both smaller than the cat and duck. The built-in ImageGen edit prompt
is `StudioLogoGroupPrompt.txt`; references are `DinosaurReference.png` and
`BearReference.png`. The previous logo is retained as `StudioLogoBeforeGroup.png`.
These are user-supplied references and a generated interpretation of them.

`MenuPoster.png` is the user's supplied `C:/Users/mvite/Downloads/unnamed.png`,
retained unchanged. The landing menu fits the entire portrait into the artwork
area so its title and lower sign stay visible at any display aspect ratio.

`MenuBluegrass.wav` is a PCM16 conversion of the existing project's mastered
`audio-exports/Squirrel Scramble - instrumental loop.wav`. It retains all
1,262,395 stereo frames at 48 kHz (26.2998958 seconds). Its mastering and circular
loop verification reports remain in `audio-exports`. The Unreal sound loops
and uses a separately saved music volume. On reaching the main menu it eases to
60% of its studio-logo level, preserving the saved slider value; it fades out
when playing. Run `Scripts/prepare_presentation.py` to rebuild the import WAV.

`StudioLogo.png` was created with built-in ImageGen from the user's supplied
avatar screenshot, preserved as `StudioAvatarReference.png`. The exact generation
prompt is in `StudioLogoPrompt.txt`. The transparent 2D production emblem combines
turdmonkey's gray tabby avatar and HoTsHoT's flaming black rubber duck avatar with
the exact studio name `hot_turd`. A flaming poo emoji replaces the movie reel.
User-provided avatars remain user-owned source
material; the production logo is a generated interpretation of those avatars.

`IntroSquirrelAtlas.png` is a transparent 4-column by 3-row sprite atlas generated
with built-in ImageGen. Its eight running cels, alert, reaching and startled
poses and acorn are original 2D artwork created for this sequence. Generation
and spacing-edit prompts are saved in `IntroSquirrelAtlasPrompt.txt`.

The 6.6-second intro uses only screen-space UMG artwork, text and animated
embers: anticipation, running interrupted by a rifle report before reaching the
nut, a comic backward tumble, a one-second pause, then the
studio logo. Its cel playback, movement, audio timing and fade are original project code
in `WildlineIntro.cpp`. It does not use the game's models or a cinematic camera.
It runs once per application session, with a skip control and no menu replay item.
The studio logo appears for 3.072 seconds, 60% longer than its earlier 1.92 seconds.
Each squirrel pose uses independent atlas bounds with transparent padding and
half-texel sampling guards; the original artwork and its position are preserved
without sampling another pose across the nominal grid boundaries.
The acorn stays on the ground at its original size for the entire scene. The
reaching cel is unused, and there is no pickup, hold or drop animation.

`native/Preview/hot_turd-intro.mp4` contains 198 native UI captures at 30 fps,
encoded by `Scripts/render_intro_preview.py`. The preview uses the game's own
procedural rifle samples and the same mastered bluegrass WAV, timed to the beats.
Capture with `-WildlineIntroReview -WildlineIntroFilm` to recreate the frames.

Reimport assets with `Scripts/import_presentation.py` using Unreal's Python
commandlet. All four presentation assets are in the always-cooked
`/Game/Wildline` directory. The studio texture retains its transparent alpha.
