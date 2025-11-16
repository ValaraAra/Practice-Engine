#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

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
};