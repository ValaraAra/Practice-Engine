#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec4.hpp>
#include <iostream>

class Renderer {
public:
	Renderer();
	~Renderer();
	void beginFrame();
	void endFrame();

private:
	const glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
};