#include "colorsScene.h"
#include "shader.h"
#include "renderer.h"
#include "shaderManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <imgui.h>

ColorsScene::ColorsScene(SceneManager& sceneManager, ShaderManager& shaderManager) : 
	sceneManager(sceneManager), shaderManager(shaderManager),
	shader(shaderManager.get("src/shaders/colors.vert.glsl", "src/shaders/colors.frag.glsl")) {

	// Create the VAO
	glGenVertexArrays(1, &VertexArrayObject);
	glBindVertexArray(VertexArrayObject);

	// Create the vertex data (triangle vertices and colors, first 3 are the vertices)
	static const GLfloat vertices[] = {
	   -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,	// bottom right
	   0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,		// bottom left
	   0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f,		// top
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
		6 * sizeof(GLfloat),// stride
		(void*)0			// array buffer offset
	);
	glEnableVertexAttribArray(0);

	// Set up colors attributes
	glVertexAttribPointer(
		1,								// attribute 0
		3,								// size (x, y, z)
		GL_FLOAT,						// type
		GL_FALSE,						// normalized?
		6 * sizeof(GLfloat),			// stride
		(void*)(3 * sizeof(GLfloat))	// array buffer offset
	);
	glEnableVertexAttribArray(1);

	// Unbind the VAO
	glBindVertexArray(0);
}

ColorsScene::~ColorsScene() {
	glDeleteBuffers(1, &VertexBufferObject);
	glDeleteVertexArrays(1, &VertexArrayObject);
}

void ColorsScene::render(Renderer& renderer) {
	renderer.useShader(&shader);

	glBindVertexArray(VertexArrayObject);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

void ColorsScene::gui() {
	ImGui::Begin("Controls");
	if (ImGui::Button("Back to Menu")) {
		sceneManager.setScene("Menu");
	}
	ImGui::End();
}
