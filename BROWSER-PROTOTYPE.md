# WILDLINE

A playable desktop-browser 3D prototype of a co-op hunters-versus-animals game. The first animal is a squirrel. Built with Three.js and an authoritative Node.js/WebSocket game server.

## Play

Install Node.js 22 or newer, then run:

```powershell
npm install
npm start
```

Open **http://localhost:3000**. Choose Hunter or Squirrel and enter the wild. AI fills the opposing side so both roles can be played solo. On Windows, `Launch Game.cmd` also installs missing dependencies and starts the game.

## Co-op

1. The host runs `npm start` and starts a new expedition with the room-code field empty.
2. Share the host's network URL printed in the terminal (for example `http://192.168.1.46:3000`) and the six-character expedition code shown in the game.
3. Friends on the same network open that host URL, choose either side, and enter the code. Up to eight human players can join. Hunters share captures; squirrels share deposited acorns.

`localhost` refers to each player's own machine, so friends must use the host's address. Public internet play requires hosting the Node server behind HTTPS with WebSocket support; this project is currently running locally. Windows may need to allow Node.js through the firewall on the private network.

## Controls

| Action | Hunter | Squirrel |
|---|---|---|
| Move / look | WASD / mouse | WASD / mouse |
| Sprint | Shift | Shift |
| Jump | Space | Space: long leap in travel/facing direction |
| Crouch | C | — |
| Scope / fire | Right / left mouse | — |
| Reload | R: 2.2 seconds | — |
| Thermal | T | — |
| Climb / release tree | — | E within reach of a trunk |
| Move on trunk | — | W/S up/down, A/D around |
| Launch from tree | — | Space in the direction you face |
| Menu | Esc, or the menu button | Esc, or the menu button |

Click the forest to capture the mouse. Escape releases it. The field guide and settings are available at camp.

## Rules

- Six-minute rounds. Hunters win at eight squirrel tags. Squirrels win at twelve acorns deposited at the den, or if time runs out.
- Squirrels carry up to four acorns. Walk over an acorn to collect it and enter the glowing den ring to deposit it. The squirrel minimap shows the den and acorns; opponents are hidden on the map unless the hunter's thermal is active.
- A tagged squirrel returns to spawn after five seconds and loses its carried stash.
- Thermal activates immediately for **10 seconds**, then remains unavailable until **180 seconds after activation**. Each hunter has an independent timer, verified by the server. T also brings up the zoomed scope during thermal use. Animals appear bright against a grayscale forest; solid geometry still occludes them.
- The rifle holds five tranquilizer darts. Shots have a 0.75-second minimum interval, and tree trunks block shots.
- The server owns movement, stamina, climbing, shots, pickups, cooldowns and team scores. Clients send controls, not positions. The same seeded reserve is used by every player.
- The reserve is roughly 220 metres across. Squirrels climb to the upper branches; a tree launch can clear more than 20 metres.

## Visuals and scope

The forest has physically based scanned ground/bark textures, instanced pine foliage, ferns, grass, a stream, rocks, fallen logs, mountains, a procedural daylight environment, shadows, fog, and subtle particles. Character and rifle models are procedural placeholders. The menu uses the live 3D scene. Ambient wind, bird calls, footsteps and feedback are generated with Web Audio.

This is a functional prototype, not Call of the Wild-level finished photorealism. That target needs production foliage and animal meshes, animation, authored landscapes, an expanded sound library, and a substantial optimization/art pass. Current network play is suitable for local testing; matchmaking, persistent accounts, internet deployment and competitive anti-cheat are not implemented.

Choose Medium or Low in Settings if frame rate is low. High enables shadows and denser ground vegetation. Mouse sensitivity and quality settings are saved locally. All runtime assets and dependencies are served locally; no CDN is required after installation.

## Verification

```powershell
npm test
npx playwright install chromium
npm run test:browser
```

Simulation tests cover exact thermal duration/cooldown, tree grabbing/climbing/long leaps, hunter role restrictions, shot occlusion/cadence, acorn collection/depositing, win conditions, reload and map boundaries. Network tests run a separate server and verify opposing roles share a room and state, invalid-room errors, and rejection of client position injection. Browser tests exercise menus, role selection, multiplayer joining and thermal activation/expiry. The browser tests expect `npm start` to be running on port 3000.

## Asset credits

- [Bark Brown 01](https://polyhaven.com/a/bark_brown_01) and [Forest Ground 01](https://polyhaven.com/a/forrest_ground_01), Poly Haven, CC0. The original 1K diffuse and OpenGL normal maps are bundled under `public/assets/`.
- Barlow Condensed and DM Sans fonts, SIL Open Font License. Source and licenses: [Barlow](https://github.com/google/fonts/tree/main/ofl/barlowcondensed), [DM Sans](https://github.com/google/fonts/tree/main/ofl/dmsans). Font license copies are bundled with the assets.
- All environment geometry, foliage textures, squirrel/hunter/rifle models, UI and generated audio in this prototype were created for this project.
