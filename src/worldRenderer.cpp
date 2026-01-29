#include "worldRenderer.h"
#include "shader.h"
#include <glm/gtc/matrix_transform.hpp>

WorldRenderer::WorldRenderer() {
	// Generate buffers and arrays
	glGenVertexArrays(1, &batchVAO);
	glGenBuffers(1, &quadVBO);
	glGenBuffers(1, &faceVBO);
	glGenBuffers(1, &offsetSSBO);
	glGenBuffers(1, &indirectBuffer);

	// Bind VAO
	glBindVertexArray(batchVAO);

	// Setup quad data
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(0);

	// Set up instance buffer
	glBindBuffer(GL_ARRAY_BUFFER, faceVBO);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(Face), (void*)offsetof(Face, packed));
	glEnableVertexAttribArray(1);
	glVertexAttribDivisor(1, 1);

	// Unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

WorldRenderer::~WorldRenderer() {
	glDeleteVertexArrays(1, &batchVAO);
	glDeleteBuffers(1, &quadVBO);
	glDeleteBuffers(1, &faceVBO);
	glDeleteBuffers(1, &offsetSSBO);
	glDeleteBuffers(1, &indirectBuffer);
}

void WorldRenderer::reset() {
	commands.clear();
	offsets.clear();
	faces.clear();
	faceCount = 0;
}

void WorldRenderer::addChunk(const glm::vec4& offset, std::vector<Face>&& faceData) {
	if (faceData.empty()) {
		return;
	}

	// Add command
	commands.push_back({
		4,
		static_cast<GLuint>(faceData.size()),
		0,
		static_cast<GLuint>(faceCount)
	});

	// Add offset
	offsets.push_back(offset);

	// Add face data
	faces.insert(faces.end(), faceData.begin(), faceData.end());
	faceCount += static_cast<GLsizei>(faceData.size());
}

void WorldRenderer::upload() {
	if (faces.empty()) {
		return;
	}

	// Upload face data
	glBindBuffer(GL_ARRAY_BUFFER, faceVBO);
	glBufferData(GL_ARRAY_BUFFER, faces.size() * sizeof(Face), faces.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Upload offsets
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, offsetSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, offsets.size() * sizeof(glm::vec4), offsets.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Upload commands
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, commands.size() * sizeof(DrawArraysIndirectCommand), commands.data(), GL_STREAM_DRAW);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void WorldRenderer::draw(const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material) {
	if (commands.empty()) {
		return;
	}

	// Set matrix uniforms
	shader.setUniform("model", glm::mat4(1.0f));
	shader.setUniform("view", view);
	shader.setUniform("projection", projection);

	// Calculate and set normal matrix
	glm::mat3 normal = glm::mat3(glm::transpose(glm::inverse(view)));
	shader.setUniform("normal", normal);

	// Set material uniforms
	shader.setUniform("material.ambient", material.ambient);
	shader.setUniform("material.diffuse", material.diffuse);
	shader.setUniform("material.specular", material.specular);
	shader.setUniform("material.shininess", material.shininess);

	// Bind buffer base for ssbo
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, offsetSSBO);

	// Draw everything
	glBindVertexArray(batchVAO);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
	glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, nullptr, static_cast<GLsizei>(commands.size()), 0);
	glBindVertexArray(0);
}
