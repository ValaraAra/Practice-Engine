#include "scene.h"
#include "shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

Scene::Scene() {
	renderSetup();
}

Scene::~Scene() {
	renderCleanup();
}

void Scene::renderSetup() {
	// Create the VAO
	glGenVertexArrays(1, &VertexArrayObject);
	glBindVertexArray(VertexArrayObject);

	// Create the vertex data (triangle vertices)
	static const GLfloat vertices[] = {
	   -1.0f, -1.0f, 0.0f,
	   1.0f, -1.0f, 0.0f,
	   0.0f,  1.0f, 0.0f,
	};

	// Create the VBO
	glGenBuffers(1, &VertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferObject);

	// Fill the VBO with vertex data
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Set up vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0,					// attribute 0
		3,					// size (x, y, z)
		GL_FLOAT,			// type
		GL_FALSE,			// normalized?
		0,					// stride
		(void*)0			// array buffer offset
	);

	// Unbind the VAO
	glBindVertexArray(0);

	// Load our shaders and create a shader program
	shader = std::make_unique<Shader>("src/shaders/basic.vert.glsl", "src/shaders/basic.frag.glsl");
}

void Scene::renderCleanup() {
	glDeleteBuffers(1, &VertexBufferObject);
	glDeleteVertexArrays(1, &VertexArrayObject);
}

void Scene::render() {
	shader->use();

	glBindVertexArray(VertexArrayObject);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

void Scene::update() {

}
