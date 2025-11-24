#pragma once

#include "structs.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class CubeMap {
public:
	CubeMap();
	~CubeMap();

	void draw(const glm::mat4& view, const glm::mat4& projection, class Shader& shader);

private:
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;

	unsigned int cubemapTexture;

	std::vector<glm::vec3> vertices;
	std::vector<unsigned int> indices;
	glm::vec3 color = glm::vec3(1.0f);

	void setupBuffers();
};