WILDLINE - FRIENDS BETA

Install
Run WildlineSetup.exe to download the latest beta, or Wildline-*-Setup.exe for
the self-contained offline installer. The game installs for your Windows account. Setup asks
for administrator permission only if shared Microsoft runtimes need installing.
All runtime installers are bundled in the full game installer; no additional runtime download,
Unreal Engine, Epic Games Launcher, Node.js or developer tools are needed.
Launch WILDLINE Beta from the Start menu or the optional desktop shortcut.
The launcher checks the public beta feed when opened and downloads verified
updates automatically. If offline it starts the installed game. The Start menu
also has an offline shortcut. Downloads can be several GB; completed parts are
reused when retrying. Close the game before installing an update.
Game versions live in separate folders. Setup verifies a new version before
selecting it, keeping the previous version available. On an update error the
launcher offers Play installed and Previous version when available.
You can also update or repair by running the full setup EXE again.
Your saved settings and hunter portrait are preserved. No uninstall is needed.
Uninstall through Windows Settings > Apps > Installed apps.

Requirements
64-bit Intel/AMD Windows 10 version 2004 with update .789 or newer, or Windows 11.
DirectX 12 graphics with Shader Model 6 support and current graphics drivers.
This beta uses high-quality graphics; performance on older PCs is unverified.
Setup checks Windows, disk space, DirectX 12 availability, Visual C++ runtime
and Microsoft GameInput. It installs missing/outdated runtimes and reports
failures or a required restart. GPU shader support is verified by the game at
launch. Setup cannot upgrade your graphics hardware or install GPU drivers.

Play
Choose a local player name on first launch. Your name appears in the upper right.
Click it or press Shift+Tab to see teams, rename yourself, switch teams, or join
the shared room. Switching teams puts you in that team's waiting room.
The list also shares each player's kills, deaths, food banked/picked, shot
accuracy and feeder scores. Stats reset each round and survive team changes.
Your Settings includes graphics presets, AA method/quality, textures, shadows,
lighting, reflections, effects, foliage, render scale, VSync and frame limits.
Press Apply Graphics to save those choices locally.
Click Map to browse preview thumbnails and descriptions for Blackwood,
Frostwood, Farmstead, Maple Crescent, Cedar Backyards, or Taylorsville Lake.
Choose a hunter/squirrel, then Enter Waiting Room. Select equipment at the
weapon racks and stand on the ready portal when everyone is ready.
WASD moves; mouse looks; Shift sprints; Space jumps; C or Ctrl crouches.
Hunter: left mouse fires, right mouse aims, R reloads, T toggles rifle thermal.
Squirrel: E grabs/releases a nearby tree, WASD climbs, Space launches away.
V changes camera; M changes minimap zoom; Esc opens camp/settings.
Mouse Y starts inverted; change it under Your Settings if desired.

Taylorsville Lake
The lodge door leads to a dock with moored boats, rods, bait and tackle. At the
end of the dock, E picks up a rod. Hold left mouse and release to cast, or use
E for a quick cast. Press E during a bite to hook the fish. Hold left mouse
to reel; release to ease fish surges. The green tension zone gains line fastest;
a full bar snaps the line. Catches show the bass and weight before releasing it.
R returns the rod; walking away cancels fishing. Instructions fade shortly after
pickup. Fishing keeps you in the waiting
area until you use the lodge's ready portal.
The hunting reserve has a roughly 200-meter lake, coves, a shore trail, and
submerged textured bass, rocks and
fallen limbs. Hunters swim automatically in deep water. WASD swims; look up
or down to steer forward swimming; Space rises; Ctrl dives. Rifle and shotgun
fire are disabled underwater. Squirrels cannot swim and die in the water,
then respawn on dry land without awarding a hunter kill.

Co-op
Connect all four PCs to the same Hamachi network. Anyone can Host Co-op
Expedition when no room is active. WILDLINE discovers the active room and
disables Host on other PCs; choose Join Expedition or Shift+Tab > Join Active
Network Room. Enter Waiting Room also joins an existing room when discovered.
Room discovery uses UDP 7778; game traffic uses UDP 7777. If discovery is blocked,
enter the current host's Hamachi IP manually. The host must keep the game open.
If they leave, someone else can host a new room; matches do not migrate hosts.
Allow WILDLINE through Windows Firewall on your private network when prompted.
Internet co-op needs UDP 7777 forwarded to the host, or a shared gaming VPN.
No accounts or automatic Internet NAT traversal are required for Hamachi play.

Feedback
Send the beta version from versions/<current.version>/BetaBuild.json, your Windows/GPU details, what you
did, what happened, and a screenshot when useful. Logs and crash reports are
under %LOCALAPPDATA%\Wildline\Saved\Logs and Saved\Crashes.
Settings and your hunter portrait stay in %LOCALAPPDATA%\Wildline\Saved.
Launcher logs are in %LOCALAPPDATA%\Wildline\LauncherLogs. Update installation
logs are in %LOCALAPPDATA%\Wildline\Updates/<version>/install.log.
Uninstall preserves these personal files. Logs may contain network addresses;
review them before sharing. This is a Development beta so diagnostics remain
available. Asset credits and third-party notices are installed with the game.
