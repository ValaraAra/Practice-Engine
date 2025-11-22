#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Cube {
public:
	Cube();
	~Cube();

	void draw(const glm::vec3& position, const glm::mat4& view, const glm::mat4& projection, class Shader& shader);

private:
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;

	void setupBuffers();
};