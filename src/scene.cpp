#include "scene.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

Scene::Scene() {

}

Scene::~Scene() {

}

void Scene::update() {

}

void Scene::render() {
	float time = static_cast<float>(glfwGetTime());

	float r = 0.5f * (std::sin(time * 0.5f + 0.0f) + 1.0f);
	float g = 0.5f * (std::sin(time * 0.5f + 2.0f) + 1.0f);
	float b = 0.5f * (std::sin(time * 0.5f + 4.0f) + 1.0f);

	glClearColor(r, g, b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}
