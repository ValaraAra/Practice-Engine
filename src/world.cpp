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

static const int MAX_HEIGHT = 32;

void World::draw(const glm::ivec3& worldPosition, const int renderDistance, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material, const Light& light) {
	glm::ivec3 centerChunkIndex = getChunkIndex(worldPosition);

	for (int x = -renderDistance; x <= renderDistance; x++)
	{
		for (int y = -renderDistance; y <= renderDistance; y++)
		{
			for (int z = -renderDistance; z <= renderDistance; z++)
			{
				glm::ivec3 current = centerChunkIndex + glm::ivec3(x, y, z);
				if (current.y < 0 || current.y >= MAX_HEIGHT) {
					continue;
				}

				// Current chunk center in world coordinates
				glm::vec3 chunkCenterWorld = getChunkCenterWorld(current);

				// Distance from player position to chunk center
				float distance = glm::length(chunkCenterWorld - glm::vec3(worldPosition));

				// Render distance in world units
				float maxDistance = float(renderDistance) * float(CHUNK_SIZE);

				if (distance > maxDistance) {
					continue;
				}

				Chunk& chunk = getOrCreateChunk(current);

				glm::ivec3 offset = current * CHUNK_SIZE;
				chunk.draw(offset, view, projection, shader, material, light);
			}
		}
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

glm::ivec3 World::getChunkCenterWorld(const glm::ivec3& chunkIndex) {
	glm::ivec3 center = (chunkIndex * CHUNK_SIZE) + int(CHUNK_SIZE * 0.5f);
	return center;
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
		generateChunk(chunkIndex);
	}

	return *chunks[chunkIndex];
}

void World::generateChunk(const glm::ivec3& chunkIndex) {
	Chunk& chunk = *chunks[chunkIndex];

	switch (generationType)
	{
	case GenerationType::Flat:
		if (chunkIndex.y != 0) {
			break;
		}

		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				for (int y = 0; y < 5; y++) {
					if (y < 3) {
						chunk.addVoxel(glm::ivec3(x, y, z), glm::vec3(0.5f, 0.5f, 0.5f));
					}
					else {
						chunk.addVoxel(glm::ivec3(x, y, z), glm::vec3(0.5f, 0.8f, 0.35f));
					}
				}
			}
		}

		break;
	case GenerationType::Simple:
		break;
	case GenerationType::Advanced:
		break;
	default:
		break;
	}
}
