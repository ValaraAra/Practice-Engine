#include "renderer.h"
#include "shaderManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>

static void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	}

	std::cout << std::endl;

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	}

	std::cout << std::endl;

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	}

	std::cout << std::endl << std::endl;
}

Renderer::Renderer(Window& window, ShaderManager& shaderManager, const glm::ivec2& resolution) : 
	window(window), shaderManager(shaderManager),
	defaultPostShader(shaderManager.get("src/shaders/post.vert.glsl", "src/shaders/post.frag.glsl")),
	ssaoShader(shaderManager.get("src/shaders/ssao.vert.glsl", "src/shaders/ssao.frag.glsl")),
	blurShader(shaderManager.get("src/shaders/ssao.vert.glsl", "src/shaders/blur.frag.glsl")) {

	// Enable OpenGL debug output
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);

	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

		std::cout << "OpenGL debug output enabled" << std::endl;
	}

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Enable backface culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// Setup FBO and quad
	setResolution(resolution);
	createQuad();

	// Generate ssao kernel
	generateSSAOKernel();
}

Renderer::~Renderer() {
	destroyFBO();
	destroyQuad();
	destroyGBuffer();
	destroySSAOBuffers();
}

void Renderer::beginFrame() {
	updateGlobals();

	// Bind FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Set viewport
	glViewport(0, 0, fboSize.x, fboSize.y);

	// Clear buffers
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::beginGeometry() {
	// Bind gbuffer FBO
	glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);

	// Set viewport
	glViewport(0, 0, fboSize.x, fboSize.y);

	// Clear buffers
	glClearColor(blackColor.r, blackColor.g, blackColor.b, blackColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Enable depth testing (less)
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
}

void Renderer::beginDeferred() {
	// Run ssao passes
	if (ssaoEnabled) {
		runSSAOPass();

		if (ssaoBlurEnabled) {
			runBlurPass();
		}
		else {
			glCopyImageSubData(ssaoTexture, GL_TEXTURE_2D, 0, 0, 0, 0, ssaoBlurTexture, GL_TEXTURE_2D, 0, 0, 0, 0, fboSize.x, fboSize.y, 1);
		}
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
		glViewport(0, 0, fboSize.x, fboSize.y);
		glClearColor(whiteColor.r, whiteColor.g, whiteColor.b, whiteColor.a);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
		glViewport(0, 0, fboSize.x, fboSize.y);
		glClearColor(whiteColor.r, whiteColor.g, whiteColor.b, whiteColor.a);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	// Bind main FBO
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Set viewport
	glViewport(0, 0, fboSize.x, fboSize.y);

	// Clear color buffer
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT);

	// Enable depth testing (less equal)
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

void Renderer::bindDeferred(Shader& shader) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPositionTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormalTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gAlbedoTexture);

	if (ssaoEnabled) {
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, ssaoBlurEnabled ? ssaoBlurTexture : ssaoTexture);
	}

	shader.setUniform("gPosition", 0);
	shader.setUniform("gNormal", 1);
	shader.setUniform("gAlbedo", 2);
	shader.setUniform("ssao", 3);
}

void Renderer::drawQuad() {
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void Renderer::endFrame() {
	// Set depth testing back (less)
	glDepthFunc(GL_LESS);

	// Unbind FBO
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Set viewport to window size
	glm::ivec2 windowSize = window.getSize();
	glViewport(0, 0, windowSize.x, windowSize.y);

	// Render screen quad with post-processing
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	Shader* currentPostShader = postProcessingShader ? postProcessingShader : &defaultPostShader;
	useShader(currentPostShader);
	currentPostShader->setUniform("screenTexture", 0);

	glBindVertexArray(quadVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
}

// Shader management
void Renderer::useShader(Shader* shader) {
	if (shader == nullptr) {
		throw std::runtime_error("Renderer: Shader is null.");
	}

	if (currentShader != shader) {
		currentShader = shader;
		currentShader->use();
	}

	setGlobalUniforms();
}

void Renderer::setPostProcessingShader() {
	postProcessingShader = &defaultPostShader;
}

void Renderer::setPostProcessingShader(Shader* shader) {
	postProcessingShader = shader;
}

// Utilities
void Renderer::setResolution(const glm::ivec2& newSize) {
	if (newSize.x <= 0 || newSize.y <= 0) {
		throw std::runtime_error("Renderer: Invalid resolution size.");
	}

	fboSize = newSize;

	// Recreate resolution-dependent buffers
	destroyFBO();
	destroyGBuffer();
	destroySSAOBuffers();

	createFBO();
	createGBuffer();
	createSSAOBuffers();
}

glm::ivec2 Renderer::getResolution() const {
	return fboSize;
}

float Renderer::getAspectRatio() const {
	return static_cast<float>(fboSize.x) / static_cast<float>(fboSize.y);
}

glm::mat4 Renderer::getProjectionMatrix() const {
	return glm::perspective(glm::radians(fov), getAspectRatio(), nearPlane, farPlane);
}

void Renderer::setProjectionSettings(float fov, float nearPlace, float farPlane) {
	this->fov = fov;
	this->nearPlane = nearPlace;
	this->farPlane = farPlane;
}

// Should these be per shader? How does shadertoy do it?
void Renderer::updateGlobals() {
	// Time
	currentTime = static_cast<float>(glfwGetTime());
	deltaTime = currentTime - lastTime;
	lastTime = currentTime;

	// Frame count
	frames++;
}

void Renderer::setGlobalUniforms() {
	if (currentShader == nullptr) {
		return;
	}
	
	// Set uniforms
	currentShader->setUniform("iResolution", glm::vec2(getResolution()));
	currentShader->setUniform("iTime", currentTime);
	currentShader->setUniform("iTimeDelta", deltaTime);
	currentShader->setUniform("iFrame", frames);
}

void Renderer::createFBO() {
	// FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Color texture
	glGenTextures(1, &colorTexture);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fboSize.x, fboSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

	// Depth and stencil texture
	glGenTextures(1, &depthStencilTexture);
	glBindTexture(GL_TEXTURE_2D, depthStencilTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, fboSize.x, fboSize.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthStencilTexture, 0);

	// Ensure framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("Renderer: Failed to create main framebuffer.");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::destroyFBO() {
	if (fbo != 0) {
		glDeleteFramebuffers(1, &fbo);
		fbo = 0;
	}
	if (colorTexture != 0) {
		glDeleteTextures(1, &colorTexture);
		colorTexture = 0;
	}
	if (depthStencilTexture != 0) {
		glDeleteTextures(1, &depthStencilTexture);
		depthStencilTexture = 0;
	}
}

void Renderer::createGBuffer() {
	// FBO
	glGenFramebuffers(1, &gBufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);

	// Position texture
	glGenTextures(1, &gPositionTexture);
	glBindTexture(GL_TEXTURE_2D, gPositionTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, fboSize.x, fboSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPositionTexture, 0);

	// Normal texture
	glGenTextures(1, &gNormalTexture);
	glBindTexture(GL_TEXTURE_2D, gNormalTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, fboSize.x, fboSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormalTexture, 0);

	// Albedo texture
	glGenTextures(1, &gAlbedoTexture);
	glBindTexture(GL_TEXTURE_2D, gAlbedoTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fboSize.x, fboSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoTexture, 0);

	// Set attachments
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	// Depth buffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthStencilTexture, 0);

	// Ensure framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("Renderer: Failed to create gbuffer framebuffer.");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::destroyGBuffer() {
	if (gBufferFBO != 0) {
		glDeleteFramebuffers(1, &gBufferFBO);
		gBufferFBO = 0;
	}
	if (gPositionTexture != 0) {
		glDeleteTextures(1, &gPositionTexture);
		gPositionTexture = 0;
	}
	if (gNormalTexture != 0) {
		glDeleteTextures(1, &gNormalTexture);
		gNormalTexture = 0;
	}
	if (gAlbedoTexture != 0) {
		glDeleteTextures(1, &gAlbedoTexture);
		gAlbedoTexture = 0;
	}
}

void Renderer::createSSAOBuffers() {
	// SSAO FBO
	glGenFramebuffers(1, &ssaoFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

	// SSAO texture
	glGenTextures(1, &ssaoTexture);
	glBindTexture(GL_TEXTURE_2D, ssaoTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, fboSize.x, fboSize.y, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTexture, 0);

	// Ensure framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("Renderer: Failed to create SSAO framebuffer.");
	}

	// Blur FBO
	glGenFramebuffers(1, &ssaoBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);

	// Blur texture
	glGenTextures(1, &ssaoBlurTexture);
	glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, fboSize.x, fboSize.y, 0, GL_RED, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoBlurTexture, 0);

	// Ensure framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("Renderer: Failed to create SSAO Blur framebuffer.");
	}

	// Generate noise
	std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator(ssaoNoiseSeed);

	std::vector<glm::vec3> ssaoNoise;
	ssaoNoise.reserve(ssaoNoiseTotalSize);

	for (int i = 0; i < ssaoNoiseTotalSize; i++) {
		glm::vec3 noise(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
		ssaoNoise.push_back(noise);
	}

	// Noise texture
	glGenTextures(1, &ssaoNoiseTexture);
	glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, ssaoNoiseSize, ssaoNoiseSize, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::destroySSAOBuffers() {
	// SSAO
	if (ssaoFBO != 0) {
		glDeleteFramebuffers(1, &ssaoFBO);
		ssaoFBO = 0;
	}
	if (ssaoTexture != 0) {
		glDeleteTextures(1, &ssaoTexture);
		ssaoTexture = 0;
	}

	// SSAO Blur
	if (ssaoBlurFBO != 0) {
		glDeleteFramebuffers(1, &ssaoBlurFBO);
		ssaoBlurFBO = 0;
	}
	if (ssaoBlurTexture != 0) {
		glDeleteTextures(1, &ssaoBlurTexture);
		ssaoBlurTexture = 0;
	}

	// Noise texture
	if (ssaoNoiseTexture != 0) {
		glDeleteTextures(1, &ssaoNoiseTexture);
		ssaoNoiseTexture = 0;
	}
}

void Renderer::generateSSAOKernel() {
	std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
	std::default_random_engine generator(ssaoKernelSeed);

	ssaoKernel.clear();
	ssaoKernel.reserve(ssaoKernelSize);

	for (int i = 0; i < ssaoKernelSize; i++) {
		glm::vec3 sample(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));
		sample = glm::normalize(sample) * randomFloats(generator);

		// Scale so samples are more aligned to the center 
		float scale = static_cast<float>(i) / static_cast<float>(ssaoKernelSize);
		scale = glm::mix(0.1f, 1.0f, scale * scale);
		sample *= scale;

		ssaoKernel.push_back(sample);
	}
}

void Renderer::runSSAOPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	glViewport(0, 0, fboSize.x, fboSize.y);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	useShader(&ssaoShader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPositionTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormalTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);

	ssaoShader.setUniform("gPosition", 0);
	ssaoShader.setUniform("gNormal", 1);
	ssaoShader.setUniform("texNoise", 2);

	ssaoShader.setUniform("kernelSize", ssaoKernelSize);
	ssaoShader.setUniform("radius", ssaoRadius);
	ssaoShader.setUniform("bias", ssaoBias);

	ssaoShader.setUniform("projection", getProjectionMatrix());

	for (int i = 0; i < ssaoKernelSize; i++) {
		ssaoShader.setUniform("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
	}

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void Renderer::runBlurPass() {
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glViewport(0, 0, fboSize.x, fboSize.y);
	glClear(GL_COLOR_BUFFER_BIT);

	useShader(&blurShader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ssaoTexture);

	blurShader.setUniform("blurInput", 0);
	blurShader.setUniform("radius", ssaoBlurRadius);

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void Renderer::createQuad() {
	// Quad vertices (position and texture coords)
	GLfloat quadVertices[] = {
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);

	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void Renderer::destroyQuad() {
	if (quadVAO != 0) {
		glDeleteVertexArrays(1, &quadVAO);
		quadVAO = 0;
	}
	if (quadVBO != 0) {
		glDeleteBuffers(1, &quadVBO);
		quadVBO = 0;
	}
}
