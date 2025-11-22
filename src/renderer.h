#pragma once

#include "shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec4.hpp>
#include <iostream>

class Renderer {
public:
	Renderer(GLFWwindow* window);
	~Renderer();
	void beginFrame();
	void endFrame();
	void useShader(Shader* shader);

private:
	const glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

	GLFWwindow* window = nullptr;
	Shader* currentShader = nullptr;

	float lastTime = 0.0f;
	float currentTime = 0.0f;
	float deltaTime = 0.0f;
	int frames = 0;

	void updateGlobals();
	void setGlobalUniforms();
};