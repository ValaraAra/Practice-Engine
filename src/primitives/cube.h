#pragma once

#include "structs.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Cube {
public:
	Cube();
	~Cube();

	void draw(const glm::vec3& position, const glm::mat4& view, const glm::mat4& projection, class Shader& shader, 
		const Material& material = {});

private:
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	glm::vec3 color = glm::vec3(1.0f);

	void setupBuffers();
};