#pragma once

#include "shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

class Scene {
public:
	Scene();
	~Scene();
	void update();
	void render();

private:
	void renderSetup();
	void renderCleanup();

	GLuint VertexArrayObject;
	GLuint VertexBufferObject;

	std::unique_ptr<Shader> shader;
};