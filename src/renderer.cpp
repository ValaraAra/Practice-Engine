#include "renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

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

Renderer::Renderer(Window& window) : window(window) {
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

	// Create FBO
	createFBO();
}

Renderer::~Renderer() {
	destroyFBO();
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

void Renderer::endFrame() {
	// Unbind FBO
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Set viewport to window size
	glm::ivec2 windowSize = window.getSize();
	glViewport(0, 0, windowSize.x, windowSize.y);

	// Blit FBO to screen (switch to quad rendering later)
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, fboSize.x, fboSize.y, 0, 0, windowSize.x, windowSize.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

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
	currentShader->setUniform("iResolution", getResolution());
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fboSize.x, fboSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

	// Render buffer
	glGenRenderbuffers(1, &renderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fboSize.x, fboSize.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		throw std::runtime_error("Renderer: Failed to create framebuffer.");
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
	if (renderBuffer != 0) {
		glDeleteRenderbuffers(1, &renderBuffer);
		renderBuffer = 0;
	}
}
