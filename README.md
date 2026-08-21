# tinywagon

<p align="center">
  <img src="/readme-src/tinywagonlogo.png" width="550">
</p>

A lightweight game development framework built with C++23 and OpenGL.

![C++23](https://img.shields.io/badge/C%2B%2B-23-blue?style=flat-square)
![OpenGL](https://img.shields.io/badge/OpenGL-3.3-green?style=flat-square)
![License](https://img.shields.io/badge/license-MIT-orange?style=flat-square)

---

## Features

- Simple implementations of several essential game systems
- Render shapes of any color or texture you want (square, triangles) <!-- TODO ADD MORE SHAPES -->
- Game camera system
- Import, organize and automatically splice texture atlases
- Easy implementation for custom shaders and GLSL code
- Text rendering
- Easily convert between screen-space and normalized coordinates (useful for game UI)
- Various helper methods for each major core system
- Built-in common math functions.
- **Super** easy to implement into any OpenGL codebase! (copy tinywagon folder into root)

---

## Dependencies & Build System

| Library | Purpose |
|---|---|
| [OpenGL](https://www.opengl.org/) | Graphics API |
| [GLFW](https://www.glfw.org/) | Window & input handling |
| [Glad](https://glad.dav1d.de/) | OpenGL loader |
| [GLM](https://github.com/g-truc/glm/) | OpenGL math library commonly used |
| [stb](https://github.com/nothings/stb/) | Easy image handling used internally |
| [CMake](https://cmake.org/) | C++ build system |
> Requires a compiler with **C++23** support.

---

## Getting Started
> **PLEASE FEEL FREE TO OPEN ANY [ISSUES](https://github.com/Gump0/tinywagon/issues) IF NEEDED.**

```bash
# Clone the repo
git clone https://github.com/Gump0/tinywagon.git
cd tinywagon

# Build (example with CMake)
cmake -S . build

# Compile the demo project (for example)
cmake --build build --target tinywagondemo

# Try out the demo scene :)
cd build
./tinywagondemo
```
### How To Include
Simply copy the tinywagon/ directory (both tinywagon.cpp & tinywagon.hpp) into your projects dependencies!

> *PLEASE REMEMBER THAT DEPENDENCIES INCLUDED IN THE [THIRDPARTY/](https://github.com/Gump0/tinywagon/tree/main/thirdparty) DIRECTORY ARE REQUIRED FOR TINYWAGON TO WORK.**
<br>

## Demonstration
<p>
  <img src="/readme-src/2drenderer.gif" width="800">
</p>

Easily load and render two dimensional sprites to the screen with minimal code. With the addition of texture atlases!

<p>
  <img src="/readme-src/textrenderingdemo.png" width="800">
</p>

Robust text editing with the option for custom fonts!

---

## License

This project is licensed under the [MIT License](LICENSE).

