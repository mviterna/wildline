# Hunter upgrade candidates — free assets only

Reviewed 2026-10-05. Free assets only. Paid assets and paid trials are excluded.
The user's downloaded Epic Game Animation Sample has been retargeted and
integrated: 63 captured locomotion and transition clips on the custom hunter rig.
Other listed assets remain download candidates. Only acquired and verified
assets enter the game; adding a candidate to this document does not integrate it.

Local sample project detected at
`C:/Users/mvite/Downloads/GameAnimationSample/GameAnimationSample.uproject`,
associated with UE 5.8. Selected animation-only FBX exports and original source
paths are retained in `GameAnimationSample/`; imported clips are under
`/Game/Wildline/Hunters/Animations`. The sample's UEFN-to-UE5 retargeter is used
before importing onto the hunter's existing skeleton.

## Best next download: Mixamo rifle clips

[Mixamo rifle library](https://www.mixamo.com/#/?page=1&query=rifle&type=Motion%2CMotionPack)
requires an Adobe ID to download. Adobe's [official FAQ](https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html)
confirms it is free without a subscription and permits royalty-free game use.
The live library and sign-in requirement were verified in the browser.

Confirmed titles visible in the rifle library:

- Rifle Aiming Idle — adjustable breath, aim height, direction and arm space.
- Rifle Idle — Two Hand Rifle Idle and Two Hand Lowered Gun Rifle Idle variants.
- Rifle Run — Running With Rifle Aimed and Running With Rifle Down variants.
- Rifle Walk — Walking With Rifle Down.
- Rifle Crouch Walk and Rifle Crouch Backward Walk.
- Rifle Aim To Down — aiming-to-lowered transition.
- Rifle Start Run, Rifle Run To Stop, Rifle Walk To Stop — movement transitions.
- Firing Rifle — multiple standing and movement variants.

Download on one consistent stock Mixamo character/skeleton: FBX Binary, 30 fps,
Without Skin. Select In Place for movement where available. An initial character
FBX With Skin and T-pose can help retarget setup. Use stock clips without uploading
the user's likeness. Retarget to the hunter's Manny rig; adapt generic reloads
to the sporting bolt rifle. Retain source and license records with downloads.

## Integrated locomotion: Epic Game Animation Sample

[Game Animation Sample](https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016)
is explicitly Free on Epic's Fab listing. It provides capsule-driven motion
matching, walk/run/jump/fall states and 500+ game-ready clips with UE5 mannequin
runtime retargeting. A strong free candidate for starts, stops, turns and foot
placement; it does not supply complete sporting-rifle handling. Epic/Fab login
is needed to acquire it. Create a separate compatible UE 5.8 sample project and
migrate selected content into Wildline.

[Lyra sample documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)
provides the official acquisition path for Epic's free shooter sample. The current
project already contains modern rifle direction clips under
/Game/Characters/Mannequins/Anims/Rifle. A full sample is useful for layering,
aiming and foot-placement reference; importing duplicate clips alone will not
repair poor weapon integration.

## Secondary free samples

[MoCap Online Free Animation Pack](https://www.fab.com/listings/64c53af0-dcb7-4483-9d65-5cbc84bd9a93)
has basic unarmed movement, crouch, idle and NPC samples. Its published clip list
has no rifle shoulder locomotion. Useful for retargeting/idle evaluation.

[Epic Animation Starter Pack](https://www.fab.com/listings/98ff449d-79db-4f54-9303-75486c4fb9d9)
is free UE-only prototype content. Lower priority than the existing modern rifle
clips and Game Animation Sample.

## Character and clothing

No better free, photoreal, rigged sporting-hunter replacement was verified in
this search. Free hits included static posed scans, stylized/fantasy characters,
undressed base meshes and products with uncertain pricing. Preserve the current
CC0 anatomy, likeness, portrait UVs and creator morphs while evaluating freely
licensed clothing, textures and weight painting.

## Current integration repair

The animation proxy blends neighboring direction clips with eased weights,
advances idle/airborne clips, shares vertical aim across chest/neck/head, and
anchors the world weapon to the evaluated shoulder pocket before solving both
arms. Post-evaluation publishes that same weapon transform on the game thread.
The old two fixed world-weapon positions are removed. The shotgun wrist target
fits arm reach behind the fore-end grip, and wrist rotation follows vertical aim.

The captured clips now replace the previous movement loops and add real crouch,
directional starts/stops, turns, takeoff and landing. Common gait phase and
measured stride lengths keep directional blends synchronized with capsule speed.
Terrain IK pins the heel-contact part of stance and releases the ankle for toe-off.
Footsteps follow gait contact; firing adds weapon recoil. The held rifle upper
body is layered in component space to avoid the different neutral-pelvis rotation
twisting the shoulders. Weapon switching evaluates the new stock in the same
frame, and profile changes preserve the attachment to the animated mesh.

More refined character clothing/skin weights and authored reload/bolt-hand
interaction remain production work. The free sample supplies locomotion rather
than sporting-rifle reloads.
The -WildlineHunterMotionReview flag captures ready, +/-25-degree aim, shotgun
and female views and logs shoulder/wrist contact errors without saving profiles.
`-WildlineHunterAnimationProbe` exercises real walking, sprinting, strafing,
turning, crouching, jumping, aiming, firing, weapon switching and both body meshes.
`-WildlineHunterLocomotionReview` records that same sequence. Packaged frame-rate
checks are available through `native/VerifyHunterAnimations.ps1`.

Earlier shoulder repair validation on 2026-10-05: editor compile and Windows BuildCookRun succeeded.
Editor and packaged motion reviews completed. The packaged review measured
0.00 cm shoulder-pad error in all five poses; wrist-target error ranged from
0.00 to 2.20 cm, below the smoke test's 3 cm contact tolerance. Native packaged
smoke and late-join hunter/squirrel co-op tests passed, including replicated
appearance and aim. Logs: native/hunter-shoulder-package-build.log,
native/hunter-shoulder-package-review.log, native/hunter-package-smoke.log,
native/hunter-net-host.log and native/hunter-net-client.log.

Captured-animation validation on 2026-10-05: Windows BuildCookRun succeeded.
Packaged probes passed at 30, 60 and 120 simulation fps. Across those runs the
maximum shoulder error was 0.00 cm, wrist error was 1.75 cm, and observed drift
between fully planted stance-foot samples was at most 2.54 cm. The tests confirm
moving feet actually reach full plant weight, and cover real capsule movement,
crouch walking, crouched ledge falls, physics jumps, recoil, pump movement,
weapon switching and the female mesh. These are animation stability checks,
not a claim that the game renders at those frame rates on every device.

The rendered packaged sequence passed the same checks at fixed 30 simulation
fps. Native smoke and late-join hunter/squirrel co-op passed. Results are in
`native/hunter-animation-package-verification.log`,
`native/test-results/hunter-animations/`, and
`native/hunter-capture-coop-verification.log`. The actual rendered sequence is
encoded to `native/Preview/HunterAnimations.mp4` with its simulation timestamps.
