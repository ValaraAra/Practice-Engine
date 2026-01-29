#pragma once

#include "structs.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <span>

struct DrawArraysIndirectCommand {
	GLuint vertexCount;
	GLuint instanceCount;
	GLuint firstVertex;
	GLuint baseInstance;
};

class WorldRenderer {
public:
	WorldRenderer();
	~WorldRenderer();

	void reset();
	void addChunk(const glm::vec4& offset, std::span<const Face> faceData);
	void upload();
	void draw(const glm::mat4& view, const glm::mat4& projection, class Shader& shader, const Material& material);

private:
	GLuint batchVAO;
	GLuint quadVBO, faceVBO;
	GLuint offsetSSBO;
	GLuint indirectBuffer;
	GLsizei faceCount = 0;

	std::vector<DrawArraysIndirectCommand> commands;
	std::vector<glm::vec4> offsets;
	std::vector<Face> faces;

	static constexpr glm::vec3 vertices[4] = {
		{ -0.5f, -0.5f, 0.0f },
		{  0.5f, -0.5f, 0.0f },
		{ -0.5f,  0.5f, 0.0f },
		{  0.5f,  0.5f, 0.0f }
	};
};