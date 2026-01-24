#include "mesh.h"
#include "shader.h"
#include <glm/gtc/matrix_transform.hpp>

Mesh::Mesh(std::vector<Face>&& faceData) {
	setupBuffers(faceData);
}

Mesh::~Mesh() {
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &quadVBO);
	glDeleteBuffers(1, &instanceVBO);
}

void Mesh::draw(const glm::vec3& position, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material) {
	// Create model matrix
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, position);

	// Set matrix uniforms
	shader.setUniform("model", model);
	shader.setUniform("view", view);
	shader.setUniform("projection", projection);

	// Calculate and set normal matrix
	glm::mat3 normal = glm::mat3(glm::transpose(glm::inverse(view * model)));
	shader.setUniform("normal", normal);

	// Set material uniforms
	shader.setUniform("material.ambient", material.ambient);
	shader.setUniform("material.diffuse", material.diffuse);
	shader.setUniform("material.specular", material.specular);
	shader.setUniform("material.shininess", material.shininess);

	// Draw it
	glBindVertexArray(quadVAO);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, faceCount);
	glBindVertexArray(0);
}

void Mesh::setupBuffers(const std::vector<Face>& faceData) {
	// Generate buffers and arrays
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glGenBuffers(1, &instanceVBO);

	// Setup quad data
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(0);

	// Set up instance data
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, faceData.size() * sizeof(Face), faceData.data(), GL_STATIC_DRAW);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(Face), (void*)offsetof(Face, packed));
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	// Unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Store counts
	faceCount = static_cast<GLsizei>(faceData.size());
}
