# City → Electric Train → Airplane → City Traffic

A 2D animated city scene built in **C++ with OpenGL / GLUT**. The program runs as one continuous journey: a ship leaves the harbour, an electric train pulls into the station through a storm, an airplane crosses a rainbow sky and lands, and finally the city street fills with traffic that shifts from day to night.

Everything on screen — buildings, buses, the train, the ship, the rainbow, the rain, the stars — is drawn from OpenGL primitives (quads, triangles, line strips and points). No textures or image files are used.

---

## Screenshots

### 1. Harbour city — ships on the river
![Harbour city](screenshots/Screenshot%20%28183%29.png)

### 2. Electric train easing into the station during a storm
![Electric train in storm](screenshots/Screenshot%20%28179%29.png)

### 3. Airplane crossing the rainbow
![Airplane and rainbow](screenshots/Screenshot%20%28180%29.png)

### 4. City traffic — daytime
![City traffic by day](screenshots/Screenshot%20%28181%29.png)

### 5. City traffic — night
![City traffic by night](screenshots/Screenshot%20%28182%29.png)

---

## Features

- **Scene sequence** — harbour → electric train → airplane → city traffic, each scene flowing into the next.
- **Day / night cycle** — the sky fades from blue to dark purple, the sun is replaced by a crescent moon and stars, building windows and street lamps light up.
- **Weather effects** — drifting clouds, animated rain during the storm scene, and a multi-band rainbow.
- **Vehicles** — a ship with a smoking funnel, a multi-coach electric train, cars, and the `CITY`, `BRTC` and `AIUB` buses moving at different speeds.
- **Interactive control** — the animation can be paused at any moment and individual objects (ship, airplane) can be started or stopped from the keyboard.
- **On-screen hints** — the top-left line of text always tells you which key the current scene is waiting for.

---

## Controls

| Key | Action |
| --- | --- |
| `SPACE` | Pause / resume the animation (`PAUSED — press SPACE to continue` appears on screen) |
| `S` | Send the ship off — the train starts on its own afterwards |
| `E` | Stop the ship |
| `L` | Begin the airplane's landing |
| `ESC` | Quit the program |

> The prompt in the top-left corner of the window always shows the key the current scene expects next, so you can simply follow it.

---

## Requirements

- **Code::Blocks** with the **MinGW / GCC** compiler
- **freeglut** (or GLUT) development files
- OpenGL drivers (already present on any normal Windows install)

---

## How to run

1. **Download or clone** this repository.

```bash
   git clone https://github.com/<your-username>/CG_GLUT_Project.git
```

2. **Open Code::Blocks.**

3. Click **Open an existing project** on the start page (or **File → Open…**).

4. Browse to the project folder you just downloaded and **select the `.cbp` file**
   (`CG_GLUT_Project.cbp`), then click **Open**.

5. Press **F9** (*Build and run*). The animation window opens straight away.

> The `.cbp` file already carries the linker settings, so there is nothing to configure —
> as long as freeglut is installed (see below), it builds on the first try.

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

If you ever need to rebuild the project settings by hand, these are the libraries under
**Project → Build options… → Linker settings → Link libraries**:

```
freeglut
opengl32
glu32
winmm
gdi32
```

### Running without Code::Blocks

With MinGW's `bin` folder on your `PATH`:

```bash
g++ main.cpp -o city.exe -lfreeglut -lopengl32 -lglu32 -lwinmm -lgdi32
city.exe
```

On Linux:

```bash
sudo apt install freeglut3-dev
g++ main.cpp -o city -lglut -lGLU -lGL
./city
```

---

## Project structure

```
CG_GLUT_Project/
├── CG_GLUT_Project.cbp   # Code::Blocks project file — open this one
├── main.cpp              # all drawing, animation and input handling
├── screenshots/          # images used in this README
└── README.md
```

---

## Troubleshooting

| Problem | Fix |
| --- | --- |
| `GL/glut.h: No such file or directory` | The freeglut headers were not copied into `MinGW\include\GL\`. Redo the first-time setup. |
| `undefined reference to 'glutInit'` / `'glBegin'` | The linker libraries are missing — add `freeglut`, `opengl32` and `glu32` in *Build options → Linker settings*. |
| `freeglut.dll is missing` when the program starts | Copy `freeglut.dll` into `C:\Windows\System32\` or next to the built `.exe`. |
| Code::Blocks says no compiler is set | **Settings → Compiler → Toolchain executables** and point it at your MinGW folder. |
| Window opens but stays blank | Make sure `glutSwapBuffers()` is called at the end of the display function. |
| Animation runs too fast or too slow | Adjust the delay passed to `glutTimerFunc()`. |

---

## Author

**Error Crafters** — Computer Science, American International University-Bangladesh (AIUB)  
Course project in Computer Graphics.
