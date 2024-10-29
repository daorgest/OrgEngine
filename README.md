# OrgEngine

OrgEngine is my personal project on a game engine, it will be soon to be expand upon to make a game soon enough :D

## Features

- **Vulkan Renderer** (Default)
- **Optional DirectX 11 and OpenGL Renderers** (In Development)
- **GLTF Mesh Loading** (via fastgltf)
- **FMOD-based audio support**
- **Tracy Profiler** integration
- **ImGui**

---

## Requirements

### Operating System

- **Windows 10/11** (x64)

### Software

- **CMake 3.28+**
- **Vulkan SDK** (Download [here](https://vulkan.lunarg.com/sdk/home))
- **MSVC** (Microsoft Visual Studio 2022 recommended)

### Third-party Libraries

These libraries are managed in the `libs/3rdParty` folder and are automatically included in the build:

- **Tracy** (for profiling)
- **fmt** (string formatting)
- **glm** (math library)
- **fastgltf** (GLTF loading)
- **FMOD** (audio support)
- **VulkanMemoryAllocator** (for Vulkan memory management)
- **vk-bootstrap** (simplified Vulkan initialization)
- **SDL3** (optional for OpenGL renderer)
- **GLAD** (OpenGL function loader)

---

## Build Instructions

### Prerequisites

1. Install the **Vulkan SDK** from [here](https://vulkan.lunarg.com/sdk/home).
2. Install **CMake 3.28+** from [here](https://cmake.org/download/).
3. Ensure you have a recent version of **Microsoft Visual Studio** with **C++ Desktop Development Tools** installed (2022 is recommended).

### Clone the Repository

To get the code and all the necessary submodules (third-party libraries), run the following in your terminal:

```bash
git clone --recurse-submodules https://github.com/daorgest/OrgEngine.git
cd OrgEngine
```

If you've already cloned the repository without submodules, you can initialize and update them with:

```bash
git submodule update --init --recursive
```

### Building the Project

1. Run CMake to configure and generate the project files:

   ```bash
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   ```

   This will configure the project and set up the build environment for **MSVC**.

2. Build the project using **MSVC**:

   ```bash
   cmake --build build --config Release
   ```

   This command builds the engine in Release mode. You can also build in Debug mode by replacing `Release` with `Debug`.

3. Once the build is complete, the executable and necessary files will be located in the `build/App` directory.

---

## Running OrgEngine

After building, you can run the engine executable from the `build/App` directory:

```bash
cd build/App
./OrgEngine.exe
```

Ensure the required assets (models, shaders, and audio) are in place by following the **"Asset Management"** section below.

---

##Renderer Configurations

By default, the engine is set to use Vulkan. The other options will be built in the future.

```cmake
# In CMakeLists.txt
option(USE_DIRECTX11 "Use DirectX 11 Renderer" OFF)
option(USE_VULKAN "Use Vulkan Renderer" ON)
option(USE_OPENGL "Use OpenGL Renderer" OFF)
```

Simply set the renderer you wish to use to `ON`, and the others to `OFF`.

### Rebuilding with a Different Renderer

After modifying the renderer option, clean the CMake cache and rebuild:

```bash
cmake --build build --target clean
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

---

## Asset Management

**To run the engine properly**, you'll need to add an **Assets** folder with specific content. During the build, ensure the following files are placed in their respective directories:

- **Models**: In the `build/App/Models` folder, add a file named `structure.glb`. This is a GLTF model that the engine will load.
- **Audio**: In the `build/App/Audio` folder, add a file named `mementomori.mp3`. This is the background audio for the engine.

The build process will automatically create the necessary folders for you. However, the assets themselves are not included in the repository as this project is still in development.

Make sure these files are in place before running the engine.

---

### Missing FMOD DLLs

Ensure the FMOD DLLs are copied to the executable directory (`build/App`) after building. If the files are missing, check the `copy_dlls_and_create_audio_folder` function in the CMake setup.

