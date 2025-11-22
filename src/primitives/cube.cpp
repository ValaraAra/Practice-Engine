#include "cube.h"
#include "shader.h"
#include <glm/gtc/matrix_transform.hpp>

Cube::Cube() {
	setupBuffers();
}

Cube::~Cube() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

// Maybe add scaling later? (for LODs or something)
void Cube::draw(const glm::vec3& position, const glm::mat4& view, const glm::mat4& projection, Shader& shader) {
	// Create model matrix
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);

	// Set uniforms
	shader.setUniform("model", model);
	shader.setUniform("view", view);
	shader.setUniform("projection", projection);

	// Draw it
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Cube::setupBuffers() {
	// Define the vertices (position and color)
	GLfloat vertices[] = {
		-0.5f, -0.5f, -0.5f,  1.0f, 0.00f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.65f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.00f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.00f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.00f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.5f, 0.00f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.4f, 0.30f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.00f, 0.7f
	};

	// Define the indices (12 triangles, arranged by face)
	GLuint indices[] = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4,
		4, 5, 1, 1, 0, 4,
		6, 7, 3, 3, 2, 6,
		4, 7, 3, 3, 0, 4,
		1, 5, 6, 6, 2, 1
	};

	// Generate and bind buffers and arrays
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind VAO and buffers
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	// Fill buffers with data
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Set up vertex attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Unbind the VAO
	glBindVertexArray(0);
}
