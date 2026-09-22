# From Nature to Urban Life

An animated OpenGL / GLUT scene in C++ that travels through four connected chapters —
a riverside town, an electric train arriving through a storm, an airplane landing under a
rainbow, and a city street that fades from day into night.

Everything on screen is drawn from OpenGL primitives (polygons, triangle fans, line strips).
No textures or sprite files are used. Sound is played through the Windows MCI API.

---

## Screenshots

### Scene 1 — Riverside town
![Riverside town](screenshots/Screenshot%20%28183%29.png)

### Scene 2 — Electric train easing into the station through a storm
![Electric train in storm](screenshots/Screenshot%20%28179%29.png)

### Scene 3 — Airplane and rainbow after the rain clears
![Airplane and rainbow](screenshots/Screenshot%20%28180%29.png)

### Scene 4 — City traffic by day
![City traffic by day](screenshots/Screenshot%20%28181%29.png)

### Scene 4 — City traffic by night
![City traffic by night](screenshots/Screenshot%20%28182%29.png)

---

## The four scenes

| # | Scene | What happens |
| --- | --- | --- |
| 1 | **Riverside town** | Windmills turn, ships sit at the bank, shops and tea stalls line the road. Press `S` and the ship sails off — which carries you into Scene 2. |
| 2 | **Electric train** | A three-coach electric train runs under catenary wires. Press `R` to bring on the storm; rain, lightning and a smooth deceleration into City Station follow, then Scene 3 begins. |
| 3 | **Airplane & rainbow** | The storm fades, a seven-band rainbow appears and birds flap past. Press `L` and the plane banks down to land, ending in Scene 4. |
| 4 | **City traffic** | A skyline, street lights and six vehicles — cars plus the `CITY`, `BRTC` and `AIUB` buses. The sky drifts from day to night on its own; press `E` to dock the ship. |

---

## Features

- **Four linked scenes** that flow into one another automatically, with manual skip and rewind.
- **Day / night cycle** — sky gradient, sun setting, moon and 50 stars, lit windows and street-lamp glows.
- **Weather system** — 160 rain drops, randomised lightning bolts with a full-screen flash, and a rainbow that fades in as the storm clears.
- **Animated vehicles** — an electric train with turning wheels and a working pantograph, a ship with a wake trail, cars and three named buses.
- **Sound** — looping ambience per scene (ship, train, thunder, plane, city) with pause/resume support.
- **Extras** — soft shadows under moving objects, animated water ripples, blinking building windows, and spinning windmills at three speeds.

---

## Controls

Eleven keys in total. The top-left corner of the window always shows the one the current
scene is waiting for, and the bottom line lists the toggles — so you never have to memorise them.

### Story keys — these advance the animation

| Key | Scene | Action |
| --- | --- | --- |
| `S` | 1 | Send the ship off (the train then starts on its own) |
| `R` | 2 | Bring on the storm — the train begins easing into the station |
| `L` | 3 | Begin the airplane's landing |
| `E` | 4 | Stop the ship in the city |

### Toggle keys — use them any time

| Key | Action |
| --- | --- |
| `N` | Day ⇄ night (Scene 4) |
| `W` | Windmill speed — stopped → normal → fast |
| `B` | City lights on / off |
| `T` | Traffic stop / go (Scene 4) |

### Playback

| Key | Action |
| --- | --- |
| `SPACE` | Pause / resume (animation **and** sound) |
| `←` | Rewind — steps backwards, even across scene boundaries |
| `→` | Skip forward to the next scene |
| `ESC` | Close the sound devices and exit |

---

## Requirements

- **Code::Blocks** with the **MinGW / GCC** compiler
- **freeglut** (or GLUT) development files
- Windows — the project uses `windows.h` and `mmsystem.h` (MCI) for audio

---

## How to run

1. **Download or clone** this repository — [FaysalMahmud313/CG_GLUT_Project](https://github.com/FaysalMahmud313/CG_GLUT_Project)

```bash
   git clone https://github.com/FaysalMahmud313/CG_GLUT_Project.git
```

2. **Open Code::Blocks.**

3. Click **Open an existing project** on the start page (or **File → Open…**).

4. Browse to the project folder and **select the `.cbp` file** (`CG_GLUT_Project.cbp`), then click **Open**.

5. Press **F9** (*Build and run*). The window titled *From Nature to Urban Life* opens at 1000 × 500.

### Linker settings

The `.cbp` already carries these, but if you build a fresh project, add them under
**Project → Build options… → Linker settings → Link libraries**:

```
freeglut
opengl32
glu32
winmm
gdi32
```

> **`winmm` is required** — without it the build fails on `mciSendStringA` and the sound
> will not compile.

### First-time only — installing freeglut for MinGW

Skip this if GLUT already works on your machine.

1. Download **freeglut for MinGW** from <https://www.transmissionzero.co.uk/software/freeglut-devel/> and unzip it.

2. Copy the files into your MinGW folder (usually `C:\Program Files\CodeBlocks\MinGW\`):

   | From the freeglut package | Copy to |
   | --- | --- |
   | `include\GL\*.h` | `MinGW\include\GL\` |
   | `lib\libfreeglut.a`, `lib\libfreeglut_static.a` | `MinGW\lib\` |
   | `bin\freeglut.dll` | `C:\Windows\System32\` (and `SysWOW64` on 64-bit Windows) |

3. Restart Code::Blocks.

### Building from the terminal instead

With MinGW's `bin` folder on your `PATH`:

```bash
g++ main.cpp -o city.exe -lfreeglut -lopengl32 -lglu32 -lwinmm -lgdi32
city.exe
```

---

## Sound setup

Audio is optional — the program runs silently if no clips are found.

Put a **`Sound`** folder next to the executable (the program also looks one, two and three
levels up, so `Sound/` beside `main.cpp` works while running from Code::Blocks). Drop in
`.wav` or `.mp3` files whose **names contain** these keywords:

| Keyword in filename | Used for |
| --- | --- |
| `cruise` | Ship engine loop |
| `train` | Train ambience (Scene 2) |
| `thunder` | Storm loop |
| `small-air` | Airplane ambience (Scene 3) |
| `landing` | One-shot landing sound |
| `horn` | City ambience (Scene 4) |

For example `cruise-ship-loop.wav`, `thunder-storm.mp3`. Matching is case-insensitive and
the first match wins.

---

## Project structure

```
CG_GLUT_Project/
├── CG_GLUT_Project.cbp   # Code::Blocks project file — open this one
├── main.cpp              # all drawing, animation, input and audio
├── Sound/                # optional audio clips (see above)
├── screenshots/          # images used in this README
└── README.md
```

---

## Code organisation

`main.cpp` is split into five authored sections, each with its own function prefix:

| Prefix | Author | Responsibility |
| --- | --- | --- |
| `T_` | Tonmoy | Sky, sun, moon, stars, clouds, rainbow, day/night |
| `M_` | Mridul | Hills, grass, water, river bank, railway track, roads, windmills |
| `F_` | Faysal | Buildings, shops, skyline, station, street lights |
| `R_` | Rumi | Train, coaches, cars and buses, traffic |
| `E_` | Epu | Ship, airplane, birds, rain, lightning, HUD |

Each section exposes one entry point (`Tonmoy_SkyAndBackground()`, `Mridul_LandWaterAndTrack()`,
and so on) and `display()` calls them back-to-front.

---

## Troubleshooting

| Problem | Fix |
| --- | --- |
| `GL/glut.h: No such file or directory` | The freeglut headers were not copied into `MinGW\include\GL\`. Redo the first-time setup. |
| `undefined reference to 'glutInit'` / `'glBegin'` | Add `freeglut`, `opengl32` and `glu32` under *Build options → Linker settings*. |
| `undefined reference to 'mciSendStringA'` | `winmm` is missing from the link libraries. |
| `freeglut.dll is missing` on launch | Copy `freeglut.dll` into `C:\Windows\System32\` or next to the built `.exe`. |
| Runs, but no sound | The `Sound` folder was not found, or the filenames don't contain the keywords listed above. |
| Code::Blocks says no compiler is set | **Settings → Compiler → Toolchain executables** and point it at your MinGW folder. |
| Animation too fast or too slow | Change the `16` in `glutTimerFunc(16, update, 0)` — it's the frame delay in milliseconds. |

---

## Authors

Computer Graphics course project — **American International University-Bangladesh (AIUB)**

Tonmoy · Mridul · Faysal · Rumi · Epu
