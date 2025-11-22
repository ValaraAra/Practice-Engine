#include "cubeScene.h"
#include "shader.h"
#include "renderer.h"
#include "shaderManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

CubeScene::CubeScene(SceneManager& sceneManager, ShaderManager& shaderManager) : sceneManager(sceneManager), shaderManager(shaderManager) {
	// Create the VAO
	glGenVertexArrays(1, &VertexArrayObject);
	glBindVertexArray(VertexArrayObject);

	// Create the vertex data (triangle vertices and colors, first 3 are the vertices)
	static const GLfloat vertices[] = {
		-1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, -1.0f, 1.0f, 0.65f, 0.0f,
		1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 0.5f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 0.4f, 0.3f, 0.0f,
		-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.7f
	};

	// Create the VBO
	glGenBuffers(1, &VertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferObject);

	// Fill the VBO with vertex data
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Create the indice data
	unsigned int indices[] = {
		0, 1, 2,
		2, 3, 0,
		4, 6, 5,
		6, 4, 7,
		0, 3, 7,
		7, 4, 0,
		1, 5, 6,
		6, 2, 1,
		0, 4, 5,
		5, 1, 0,
		3, 2, 6,
		6, 7, 3
	};

	// Create the EBO
	glGenBuffers(1, &ElementBufferObject);

	// Fill the EBO with indice data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Set up vertex attributes
	glVertexAttribPointer(
		0,					// attribute
		3,					// size (x, y, z)
		GL_FLOAT,			// type
		GL_FALSE,			// normalized?
		6 * sizeof(GLfloat),// stride
		(void*)0			// array buffer offset
	);
	glEnableVertexAttribArray(0);

	// Set up colors attributes
	glVertexAttribPointer(
		1,								// attribute
		3,								// size (x, y, z)
		GL_FLOAT,						// type
		GL_FALSE,						// normalized?
		6 * sizeof(GLfloat),			// stride
		(void*)(3 * sizeof(GLfloat))	// array buffer offset
	);
	glEnableVertexAttribArray(1);

	// Unbind the VAO
	glBindVertexArray(0);

	// Load our shaders and create a shader program
	shader = &shaderManager.get("src/shaders/simple.vert.glsl", "src/shaders/simple.frag.glsl");
}

CubeScene::~CubeScene() {
	glDeleteBuffers(1, &VertexBufferObject);
	glDeleteBuffers(1, &ElementBufferObject);
	glDeleteVertexArrays(1, &VertexArrayObject);
}

void CubeScene::render(Renderer& renderer) {
	renderer.useShader(shader);

	// Transformation matrices
	glm::mat4 model = glm::mat4(1.0f);

	// Camera position, target, and up vector
	glm::mat4 view = glm::lookAt(
		glm::vec3(4, 3, 3),
		glm::vec3(0, 0, 0),
		glm::vec3(0, 1, 0)
	);

	// FOV, aspect ratio, near plane, and far plane
	glm::mat4 projection = renderer.getProjectionMatrix();

	// Set uniforms
	shader->setUniform("model", model);
	shader->setUniform("view", view);
	shader->setUniform("projection", projection);

	// Draw
	glBindVertexArray(VertexArrayObject);
	glDrawElements(GL_TRIANGLES, 12*3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void CubeScene::gui() {
	ImGui::Begin("Controls");
	if (ImGui::Button("Back to Menu")) {
		sceneManager.setScene("Menu");
	}
	ImGui::End();
}
