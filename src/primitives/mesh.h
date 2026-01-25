#pragma once

#include "structs.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Mesh {
public:
	Mesh(std::vector<Face>&& faceData);
	~Mesh();

	void draw(const glm::vec3& position, const glm::mat4& view, const glm::mat4& projection, class Shader& shader, const Material& material);

private:
	GLuint instanceVBO;
	GLuint quadVAO, quadVBO;
	GLsizei faceCount;

	static constexpr glm::vec3 vertices[4] = {
		{ -0.5f, -0.5f, 0.0f },
		{  0.5f, -0.5f, 0.0f },
		{ -0.5f,  0.5f, 0.0f },
		{  0.5f,  0.5f, 0.0f }
	};

	void setupBuffers(const std::vector<Face>& faceData);
};