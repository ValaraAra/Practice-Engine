#pragma once

#include "shader.h"
#include "window.h"
#include "shaderManager.h"
#include <glm/vec4.hpp>
#include <iostream>

class Renderer {
public:
	Renderer(Window& window, ShaderManager& shaderManager, const glm::ivec2& resolution);
	~Renderer();

	void beginFrame();
	void endFrame();
	void useShader(Shader* shader);
	void setPostProcessingShader();
	void setPostProcessingShader(Shader* shader);

	glm::mat4 getProjectionMatrix() const;
	void setProjectionSettings(float fov, float nearPlace, float farPlane);

	void setResolution(const glm::ivec2& newSize);
	glm::ivec2 getResolution() const;
	float getAspectRatio() const;

	float getFOV() const { return fov; }
	float getNearPlane() const { return nearPlane; }
	float getFarPlane() const { return farPlane; }
	float getDeltaTime() const { return deltaTime; }

private:
	const glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

	Window& window;
	ShaderManager& shaderManager;

	Shader* currentShader = nullptr;
	Shader* postProcessingShader = nullptr;
	Shader& defaultPostShader;

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

	// Quad
	GLuint quadVAO = 0;
	GLuint quadVBO = 0;

	void updateGlobals();
	void setGlobalUniforms();

	void createFBO();
	void destroyFBO();

	void createQuad();
	void destroyQuad();
};