#include "renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/vec4.hpp>
#include <iostream>



Renderer::Renderer() {

}

Renderer::~Renderer() {

}

void Renderer::beginFrame() {
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::endFrame() {

}
