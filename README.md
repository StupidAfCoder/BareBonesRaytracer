# BareBonesRaytracer

A software raytracer written in C++ with a real-time interactive preview powered by SDL3 and Dear ImGui. Built from scratch as a learning project, covering the core theory of ray-sphere intersection, diffuse and specular shading, shadows, and reflections.

---

## Demo

<!-- Attach your video here. You can embed it one of two ways:
     1. GitHub-hosted video (drag and drop the file into this README on GitHub.com):
        GitHub will generate a link like: https://github.com/user-attachments/assets/<uuid>.mp4
        Then embed it as:
        https://github.com/user-attachments/assets/<uuid>.mp4

     2. YouTube / external host:
        [![Demo](https://img.youtube.com/vi/YOUR_VIDEO_ID/0.jpg)](https://www.youtube.com/watch?v=YOUR_VIDEO_ID)
-->

---

## Renders

| Scene | Output |
|-------|--------|
| Complete scene with reflections | ![complete](BareBonesRaytracer/Output/complete.png) |
| Shadow pass | ![shadows](BareBonesRaytracer/Output/shadows.png) |
| Single shadow test | ![shadow](BareBonesRaytracer/Output/shadow.png) |

---

## Features

- Ray-sphere intersection using the quadratic formula
- Diffuse (Lambertian) and specular (Phong) shading
- Hard shadows with shadow-ray bias to prevent acne
- Mirror reflections with configurable depth (default: 3 bounces)
- Three light types: ambient, point, and directional
- Multithreaded rendering using `std::thread`, one band per hardware thread
- Real-time SDL3 preview with live ImGui controls for camera, lights, and spheres
- Export to PNG (via stb_image_write) and PPM
- Camera with yaw/pitch rotation via a 3x3 rotation matrix

---

## Dependencies

| Library | How it is provided |
|---------|--------------------|
| SDL3 | Auto-downloaded via CMake FetchContent |
| Dear ImGui | Vendored in `external/ImgGui/` |
| stb_image_write | Single-header in `external/` |

No manual dependency installation is required.

---

## Build

An internet connection is required — CMake fetches SDL3 automatically during the configure step. All other dependencies are vendored in the repo.

---

### Windows

**1. Install prerequisites**

- [Visual Studio 2019 or newer](https://visualstudio.microsoft.com/downloads/) — during install, select the **Desktop development with C++** workload. This includes MSVC and CMake.
- [Git for Windows](https://git-scm.com/download/win)

**2. Clone and build**

Open **Developer Command Prompt for VS** (search for it in the Start menu — a regular terminal will not have the compiler on its PATH).

```bat
git clone https://github.com/<your-username>/BareBonesRaytracer.git
cd BareBonesRaytracer\BareBonesRaytracer
cmake -S . -B build
cmake --build build --config Release
```

**3. Run**

```bat
build\Release\BareBonesRaytracer.exe
```

> If you prefer the Visual Studio GUI: after the `cmake -S . -B build` step, open `build\BareBonesRaytracer.sln`, set the configuration to **Release**, and press **Ctrl+F5**.

---

### macOS

**1. Install prerequisites**

Install Xcode Command Line Tools if you have not already:

```bash
xcode-select --install
```

Install CMake via Homebrew (install [Homebrew](https://brew.sh) first if needed):

```bash
brew install cmake
```

**2. Clone and build**

```bash
git clone https://github.com/<your-username>/BareBonesRaytracer.git
cd BareBonesRaytracer/BareBonesRaytracer
cmake -S . -B build
cmake --build build
```

**3. Run**

```bash
./build/BareBonesRaytracer
```

---

### Linux (Ubuntu / Debian)

**1. Install prerequisites**

```bash
sudo apt update
sudo apt install -y git cmake build-essential \
    libx11-dev libxext-dev libxi-dev libxrandr-dev \
    libxcursor-dev libxinerama-dev libwayland-dev \
    libxkbcommon-dev libasound2-dev libpulse-dev \
    libgl1-mesa-dev
```

> These are the system libraries SDL3 needs to compile against. The build will fail with missing-header errors if they are not present.

**2. Clone and build**

```bash
git clone https://github.com/<your-username>/BareBonesRaytracer.git
cd BareBonesRaytracer/BareBonesRaytracer
cmake -S . -B build
cmake --build build
```

**3. Run**

```bash
./build/BareBonesRaytracer
```

---

### Other Linux distros

The apt package names above map to the following on other package managers:

| Fedora / RHEL | Arch |
|---------------|------|
| `dnf install cmake gcc-c++ libX11-devel libXext-devel libXi-devel libXrandr-devel libXcursor-devel libXinerama-devel wayland-devel libxkbcommon-devel alsa-lib-devel pulseaudio-libs-devel mesa-libGL-devel` | `pacman -S cmake base-devel libx11 libxext libxi libxrandr libxcursor libxinerama wayland libxkbcommon alsa-lib pulseaudio mesa` |

---

## Controls

Open the **Controls** panel on the right side of the window.

| Section | What it does |
|---------|--------------|
| Camera Controls | Move the camera position and adjust yaw / pitch |
| Lights | Reposition and change intensity of point, directional, and ambient lights |
| Spheres | Select a sphere to edit its position, radius, color, specular, and reflectivity. Add or remove spheres (max 20) |
| Save PNG / Save PPM | Write the current framebuffer to `Output/` |

Any change triggers an automatic re-render. An in-progress render is cancelled and restarted cleanly.

---

## Project Structure

```
BareBonesRaytracer/
├── src/
│   ├── main.cpp          # Renderer, scene setup, SDL3 + ImGui loop
│   └── stb_impl.cpp      # STB single-header implementation unit
├── include/
│   └── models.h          # Vec3D, Point3D, Color, Mat3D, Sphere, Light, IntersectData
├── external/
│   ├── ImgGui/           # Dear ImGui source + SDL3 backend
│   ├── stb_image_write.h
│   └── imgui.ini
├── Output/               # Saved renders land here
└── CMakeLists.txt
```

---

## Known Limitations

- Spheres only — no triangle meshes or BVH
- No anti-aliasing
- Reinhard tone mapping is implemented but currently disabled
- Scene is defined in code; no scene file format
- Light count is hardcoded to three entries; the ImGui light panel assumes this layout

---

## References

- [Computer Graphics from Scratch — Gabriel Gambetta](https://gabrielgambetta.com/computer-graphics-from-scratch/)
- [Ray Tracing in One Weekend — Peter Shirley](https://raytracing.github.io/)
