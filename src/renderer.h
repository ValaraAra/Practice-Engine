#pragma once

#include "shader.h"
#include "window.h"
#include <glm/vec4.hpp>
#include <iostream>

class Renderer {
public:
	Renderer(Window& window);
	~Renderer();
	void beginFrame();
	void endFrame();
	void useShader(Shader* shader);

	glm::mat4 getProjectionMatrix() const;
	void setProjectionSettings(float fov, float nearPlace, float farPlane);

	glm::ivec2 getResolution() const;
	float getAspectRatio() const;
	float getFOV() const { return fov; }
	float getNearPlane() const { return nearPlane; }
	float getFarPlane() const { return farPlane; }
	float getDeltaTime() const { return deltaTime; }

private:
	const glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

	Window& window;
	Shader* currentShader = nullptr;

	// Timing
	float lastTime = 0.0f;
	float currentTime = 0.0f;
	float deltaTime = 0.0f;
	int frames = 0;

	// Projection settings
	float fov = 45.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	// FBO
	GLuint fbo = 0;
	GLuint colorTexture = 0;
	GLuint renderBuffer = 0;
	glm::ivec2 fboSize = glm::ivec2(1920, 1080);

	void updateGlobals();
	void setGlobalUniforms();
	void createFBO();
	void destroyFBO();
};