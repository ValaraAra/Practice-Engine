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
	void beginGeometry();
	void beginDeferred();
	void bindDeferred(Shader& shader);
	void drawQuad();
	void endFrame();

	void useShader(Shader* shader);
	void setPostProcessingShader();
	void setPostProcessingShader(Shader* shader);

	glm::mat4 getProjectionMatrix() const;
	void setProjectionSettings(float fov, float nearPlace, float farPlane);

	void setResolution(const glm::ivec2& newSize);
	glm::ivec2 getResolution() const;
	float getAspectRatio() const;

	void setSSAOEnabled(bool enabled) { ssaoEnabled = enabled; }
	void setSSAOBlurEnabled(bool enabled) { ssaoBlurEnabled = enabled; }
	void setSSAOKernelSize(int size) { if (ssaoKernelSize == size) return; ssaoKernelSize = size; generateSSAOKernel(); }
	void setSSAONoiseSize(int size) { if (ssaoNoiseSize == size) return; ssaoNoiseSize = size; ssaoNoiseTotalSize = ssaoNoiseSize * ssaoNoiseSize; createSSAOBuffers(); }
	void setSSAOBlurRadius(int radius) { ssaoBlurRadius = radius; }
	void setSSAORadius(float radius) { ssaoRadius = radius; }
	void setSSAOBias(float bias) { ssaoBias = bias; }

	float getFOV() const { return fov; }
	float getNearPlane() const { return nearPlane; }
	float getFarPlane() const { return farPlane; }
	float getDeltaTime() const { return deltaTime; }

private:
	const glm::vec4 blackColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	const glm::vec4 whiteColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	const glm::vec4 clearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);

	Window& window;
	ShaderManager& shaderManager;

	Shader* currentShader = nullptr;
	Shader* postProcessingShader = nullptr;
	Shader& defaultPostShader;
	Shader& ssaoShader;
	Shader& blurShader;

	// Timing
	float lastTime = 0.0f;
	float currentTime = 0.0f;
	float deltaTime = 0.0f;
	int frames = 0;

	// Projection settings
	float fov = 45.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	// FBO settings
	glm::ivec2 fboSize = glm::ivec2(1920, 1080);

	// Main
	GLuint fbo = 0;
	GLuint colorTexture = 0;
	GLuint depthStencilTexture = 0;

	// GBuffer
	GLuint gBufferFBO = 0;
	GLuint gPositionTexture = 0;
	GLuint gNormalTexture = 0;
	GLuint gAlbedoTexture = 0;

	// SSAO settings
	int ssaoKernelSize = 64;
	int ssaoNoiseSize = 4;
	int ssaoNoiseTotalSize = ssaoNoiseSize * ssaoNoiseSize;
	int ssaoBlurRadius = 1;
	float ssaoRadius = 1.0f;
	float ssaoBias = 0.025f;

	bool ssaoEnabled = true;
	bool ssaoBlurEnabled = true;

	unsigned int ssaoKernelSeed = 123u;
	unsigned int ssaoNoiseSeed = 321u;

	// SSAO
	GLuint ssaoFBO = 0;
	GLuint ssaoTexture = 0;

	GLuint ssaoBlurFBO = 0;
	GLuint ssaoBlurTexture = 0;

	GLuint ssaoNoiseTexture = 0;
	std::vector<glm::vec3> ssaoKernel;

	// Screen quad
	GLuint quadVAO = 0;
	GLuint quadVBO = 0;

	void updateGlobals();
	void setGlobalUniforms();

	void createFBO();
	void destroyFBO();

	void createGBuffer();
	void destroyGBuffer();

	void createSSAOBuffers();
	void destroySSAOBuffers();

	void generateSSAOKernel();

	void runSSAOPass();
	void runBlurPass();

	void createQuad();
	void destroyQuad();
};