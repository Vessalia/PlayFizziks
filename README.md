# PlayFizziks
Demo of the [Fizziks](https://github.com/Vessalia/Fizziks) physics engine.

## Usage
- A basic UI using [imgui](https://github.com/ocornut/imgui) exposes the ability to change some basic settings on the world, as well as spawning and clearing rigid-bodies. This includes all 5 basic shape types, along with a polygon builder for complex shapes.
- Scene saving and loading is made available via platform agnostic [file dialog](https://github.com/btzy/nativefiledialog-extended), and [json serialization](https://github.com/nlohmann/json).
  - Scenes loaded are initially paused. Furthermore, forces and warm-starting are not preserved, so loading scenes with a large number, or strongly acting interactions, may behave unpredictably.
- An examples folder can be found under `assets/examples`, showing some prebuilt scenes for interaction.

## Building
- C++20 compatible compiler
- CMake 3.28 - 4.2
- Only tested on Windows

### Build Steps
PlayFizziks is built using CMake
```bash
git clone https://github.com/Vessalia/PlayFizziks.git
cd PlayFizziks
mkdir build
cmake -S . -B build
cmake --build build
```

## Notes
- This version uses an incomplete version of the Fizziks library. While a fully functional Fizziks engine, some issues/odd behaviour is likely to occur. Be mindful of this especially when building concave polygons, as contact manifolds are not handled in the collision resolution in this iteration.
