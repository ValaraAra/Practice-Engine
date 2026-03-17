# Practice Engine

A practice voxel engine built with modern C++ and OpenGL.

![Voxel World Screenshot](screenshot.png "Voxel World Screenshot")

## Features
- 'Infinite' Terrain with Chunking
	- Multi-Threaded Chunk Generation
	- Multi-Threaded Mesh Building
- Rendering
	- Hybrid Pipeline
		- Deferred (Opaque)
		- Forward (Translucent)
	- Lighting
		- Blinn-Phong Model
		- Directional, Point, and Spot
	- Screen Space Ambient Occlusion
	- Frustrum Culling
	- Skybox
	- Water

## Building
Tested on Windows 10 with the basic Visual Studio 2026 setup.

### Dependencies
- [GLFW](https://www.glfw.org/) - Windowing and input
- [GLM](https://glm.g-truc.net/) - OpenGL Math
- [GLAD](https://glad.dav1d.de/) - OpenGL loader
- [ImGui](https://github.com/ocornut/imgui) - GUI Library
- [STB Image](https://github.com/nothings/stb) - Image loading
- [Tracy](https://github.com/wolfpld/tracy) - Profiling

Dependencies belong in `external/`

### Resources
Skybox textures belong in `resources/textures/skybox` (top, bottom, left, right, front, back)

Models (vox) belong in `resources/models`
