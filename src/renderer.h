#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

class Renderer {
public:
	Renderer();
	~Renderer();
	void beginFrame();
	void endFrame();

private:

};