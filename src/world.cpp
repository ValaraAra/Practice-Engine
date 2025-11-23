#include "world.h"
#include "chunk.h"
#include "shader.h"
#include "primitives/mesh.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_set>
#include <memory>
#include <stdexcept>
#include <iostream>

World::World() {

}

World::~World() {

}

void World::draw(const glm::mat4& view, const glm::mat4& projection, Shader& shader) {
	for (auto& [pos, chunk] : chunks) {
		glm::ivec3 offset = pos * CHUNK_SIZE;
		chunk->draw(offset, view, projection, shader);
	}
}

bool World::hasVoxel(const glm::ivec3& worldPosition) {
	glm::ivec3 chunkIndex = getChunkIndex(worldPosition);

	if (!chunks.contains(chunkIndex)) {
		return false;
	}

	glm::ivec3 localPosition = getLocalPosition(worldPosition);
	return chunks[chunkIndex]->hasVoxel(localPosition);
}

void World::addVoxel(const glm::ivec3& worldPosition) {
	glm::ivec3 chunkIndex = getChunkIndex(worldPosition);
	Chunk& chunk = getOrCreateChunk(chunkIndex);

	glm::ivec3 localPosition = getLocalPosition(worldPosition);
	chunk.addVoxel(localPosition);
}

void World::removeVoxel(const glm::ivec3& worldPosition) {
	glm::ivec3 chunkIndex = getChunkIndex(worldPosition);

	if (!chunks.contains(chunkIndex)) {
		return;
	}

	glm::ivec3 localPosition = getLocalPosition(worldPosition);
	chunks[chunkIndex]->removeVoxel(localPosition);
}

glm::ivec3 World::getChunkIndex(const glm::ivec3& worldPosition) {
	return glm::ivec3(glm::floor(glm::vec3(worldPosition) / float(CHUNK_SIZE)));

}

glm::ivec3 World::getLocalPosition(const glm::ivec3& worldPosition) {
	glm::ivec3 localPosition = worldPosition % CHUNK_SIZE;

	// Handle negatives
	if (localPosition.x < 0) localPosition.x += CHUNK_SIZE;
	if (localPosition.y < 0) localPosition.y += CHUNK_SIZE;
	if (localPosition.z < 0) localPosition.z += CHUNK_SIZE;

	return localPosition;
}

Chunk& World::getOrCreateChunk(const glm::ivec3& chunkIndex) {
	if (!chunks.contains(chunkIndex)) {
		chunks[chunkIndex] = std::make_unique<Chunk>();
	}

	return *chunks[chunkIndex];
}
