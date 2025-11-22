#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
};

class Mesh {
public:
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
	~Mesh();

	void draw(const glm::vec3& position, const glm::mat4& view, const glm::mat4& projection, class Shader& shader);

private:
	GLuint VAO, VBO, EBO;

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	void setupBuffers();
};