#include "renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <iostream>

Renderer::Renderer(GLFWwindow* window) : window(window) {

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

	if (currentShader == shader) {
		return;
	}

	currentShader = shader;
	currentShader->use();
	setGlobalUniforms();
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