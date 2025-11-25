# Practice Engine

A simple practice engine built with C++ 23 and OpenGL 4.6 Core.

## Features
- Basic World Generation
- 'Infinite' Terrain with Chunking
- First-Person Camera
- Skybox Rendering
- Basic Lighting (Blin-Phong Model)
- GUI with ImGui

## Dependencies
- [GLFW](https://www.glfw.org/) - Windowing and input
- [GLM](https://glm.g-truc.net/) - OpenGL Math
- [GLAD](https://glad.dav1d.de/) - OpenGL loader
- [ImGui](https://github.com/ocornut/imgui) - GUI Library
- [STB Image](https://github.com/nothings/stb) - Image loading

Ensure these are cloned or installed as submodules in the `external/` directory, following the configuration in CMakeLists.txt.

## Resources
Ensure you have skybox textures in the `resources/textures/skybox` directory (top, bottom, left, right, front, back).

## Building the Project
Tested on Windows with the basic Visual Studio 2026 setup.

## Helpful Links
- [LearnOpenGL](https://learnopengl.com/) - OpenGL tutorials.
- [OpenGL-Tutorial](https://www.opengl-tutorial.org/) - More OpenGL tutorials.

## To-Do List
- Better Comments and Documentation
- Mesh Building Optimization
- Chunk Generation Optimization
- Chunk Generation Multithreading
- Implement Basic Shadows
- DDA Raycasting