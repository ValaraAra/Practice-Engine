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

Renderer::Renderer(GLFWwindow* window) : window(window) {
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
}

Renderer::~Renderer() {

}

void Renderer::beginFrame() {
	updateGlobals();

	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {

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

glm::mat4 Renderer::getProjectionMatrix() const {
	return glm::perspective(glm::radians(fov), getAspectRatio(), nearPlane, farPlane);
}

void Renderer::setProjectionSettings(float fov, float nearPlace, float farPlane) {
	this->fov = fov;
	this->nearPlane = nearPlace;
	this->farPlane = farPlane;
}

float Renderer::getAspectRatio() const {
	float aspectRatio = 1.0f;
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	if (height == 0) {
		return 1.0f;
	}

	aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	return aspectRatio;
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

	// Get resolution
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glm::vec2 resolution = glm::vec2(static_cast<float>(width), static_cast<float>(height));
	
	// Set uniforms
	currentShader->setUniform("iResolution", resolution);
	currentShader->setUniform("iTime", currentTime);
	currentShader->setUniform("iTimeDelta", deltaTime);
	currentShader->setUniform("iFrame", frames);
}