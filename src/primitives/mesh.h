#pragma once

#include "structs.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Mesh {
public:
	Mesh(std::vector<Vertex>&& vertexData, std::vector<unsigned int>&& indices);
	~Mesh();

	void draw(const glm::vec3& position, const glm::mat4& view, const glm::mat4& projection, class Shader& shader, const Material& material);

private:
	GLuint VAO, VBO, EBO;
	GLsizei vertexCount, indiceCount;

	void setupBuffers(const std::vector<Vertex>& vertexData, const std::vector<unsigned int>& indices);
};