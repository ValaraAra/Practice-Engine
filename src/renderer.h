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

	glm::mat4 getProjectionMatrix() const;
	void setProjectionSettings(float fov, float nearPlace, float farPlane);

	float getAspectRatio() const;
	float getFOV() const { return fov; }
	float getNearPlane() const { return nearPlane; }
	float getFarPlane() const { return farPlane; }
	float getDeltaTime() const { return deltaTime; }

private:
	const glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

	GLFWwindow* window = nullptr;
	Shader* currentShader = nullptr;

	float lastTime = 0.0f;
	float currentTime = 0.0f;
	float deltaTime = 0.0f;
	int frames = 0;

	float fov = 45.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	void updateGlobals();
	void setGlobalUniforms();
};