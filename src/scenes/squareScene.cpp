#include "squareScene.h"
#include "shader.h"
#include "shaderManager.h"
#include "renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <imgui.h>

SquareScene::SquareScene(SceneManager& sceneManager, ShaderManager& shaderManager) : sceneManager(sceneManager), shaderManager(shaderManager) {
	// Create the VAO
	glGenVertexArrays(1, &VertexArrayObject);
	glBindVertexArray(VertexArrayObject);

	// Create the vertex data
	static const GLfloat vertices[] = {
		// First Triangle
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f,  0.5f, 0.0f,
		// Second Triangle
		0.5f, 0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		-0.5f,  -0.5f, 0.0f,
	};

	// Create the VBO
	glGenBuffers(1, &VertexBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferObject);

	// Fill the VBO with vertex data
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Set up vertex attributes
	glVertexAttribPointer(
		0,					// attribute 0
		3,					// size (x, y, z)
		GL_FLOAT,			// type
		GL_FALSE,			// normalized?
		0,					// stride
		(void*)0			// array buffer offset
	);
	glEnableVertexAttribArray(0);

	// Unbind the VAO
	glBindVertexArray(0);

	// Load our shaders and create a shader program
	shader = &shaderManager.get("src/shaders/basic.vert.glsl", "src/shaders/basic.frag.glsl");
}

SquareScene::~SquareScene() {
	glDeleteBuffers(1, &VertexBufferObject);
	glDeleteVertexArrays(1, &VertexArrayObject);
}

void SquareScene::render(Renderer& renderer) {
	renderer.useShader(shader);

	glBindVertexArray(VertexArrayObject);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void SquareScene::gui() {
	ImGui::Begin("Controls");
	if (ImGui::Button("Back to Menu")) {
		sceneManager.setScene("Menu");
	}
	ImGui::End();
}
