#include "mesh.h"
#include "shader.h"
#include <glm/gtc/matrix_transform.hpp>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices) : vertices(std::move(vertices)), indices(std::move(indices)) {
	setupBuffers();
}

Mesh::~Mesh() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

// Maybe add scaling later? (for LODs or something)
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
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::setupBuffers() {
	// Generate VAO and buffers
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind VAO and buffers
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	// Fill buffers with data
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	// Set up vertex attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	// Unbind the VAO
	glBindVertexArray(0);
}
