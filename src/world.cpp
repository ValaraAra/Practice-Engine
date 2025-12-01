#include "world.h"
#include "chunk.h"
#include "shader.h"
#include "primitives/mesh.h"
#include <unordered_set>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/noise.hpp>
#include <array>

World::World(GenerationType generationType) : generationType(generationType) {

}

World::~World() {

}

void World::draw(const glm::ivec3& worldPosition, const int renderDistance, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material) {
	glm::ivec2 centerChunkIndex = getChunkIndex(worldPosition);

	for (int x = -renderDistance; x <= renderDistance; x++)
	{
		for (int z = -renderDistance; z <= renderDistance; z++)
		{
			glm::ivec2 current = centerChunkIndex + glm::ivec2(x, z);

			// Skip if chunk doesn't exist
			if (!chunks.contains(current)) {
				continue;
			}

			// Calculate max distance and distance to chunk center in world space
			glm::vec2 chunkCenterWorld = getChunkCenterWorld(current);
			float distanceToChunkCenterWorld = glm::length(chunkCenterWorld - glm::vec2(worldPosition.x, worldPosition.z));
			float maxDistanceWorld = float(renderDistance) * float(CHUNK_SIZE);

			// Skip if outside render distance
			if (distanceToChunkCenterWorld > maxDistanceWorld) {
				continue;
			}

			/// Draw it
			glm::ivec2 offset = current * CHUNK_SIZE;
			ChunkNeighbors& neighbors = getChunkNeighbors(current);

			chunks[current]->draw(offset, neighbors, view, projection, shader, material);
		}
	}
}

ChunkNeighbors& World::getChunkNeighbors(glm::ivec2 chunkIndex) {
	static ChunkNeighbors neighbors = {};

	if (chunks.contains(chunkIndex + directions[0])) {
		neighbors.nx = chunks[chunkIndex + directions[0]].get();
	}
	if (chunks.contains(chunkIndex + directions[1])) {
		neighbors.px = chunks[chunkIndex + directions[1]].get();
	}
	if (chunks.contains(chunkIndex + directions[2])) {
		neighbors.ny = chunks[chunkIndex + directions[2]].get();
	}
	if (chunks.contains(chunkIndex + directions[3])) {
		neighbors.py = chunks[chunkIndex + directions[3]].get();
	}

	return neighbors;
}

void World::updateGenerationQueue(const glm::ivec3& worldPosition, const int renderDistance) {
	glm::ivec2 centerChunkIndex = getChunkIndex(worldPosition);

	resetGenerationQueue();

	// Add chunks within render distance to the generation queue
	for (int x = -renderDistance; x <= renderDistance; x++)
	{
		for (int z = -renderDistance; z <= renderDistance; z++)
		{
			glm::ivec2 current = centerChunkIndex + glm::ivec2(x, z);

			if (chunks.contains(current)) {
				continue;
			}

			// Calculate distance to chunk center in world space
			glm::vec2 chunkCenterWorld = getChunkCenterWorld(current);
			float distanceToChunkCenterWorld = glm::length(chunkCenterWorld - glm::vec2(worldPosition.x, worldPosition.z));

			generationQueue.push(std::make_pair(-distanceToChunkCenterWorld, current));
		}
	}
}

// Processes the chunk generation queue, generating up to x chunks per iteration
void World::processGenerationQueue(int maxChunksPerIteration) {
	int chunksProcessed = 0;
	while (!generationQueue.empty() && chunksProcessed < maxChunksPerIteration) {
		glm::ivec2 chunkIndex = generationQueue.top().second;
		generationQueue.pop();

		// Create chunk if it doesn't exist
		if (!chunks.contains(chunkIndex)) {
			chunks[chunkIndex] = std::make_unique<Chunk>();
		}

		// Generate it
		generateChunk(chunkIndex);
		chunksProcessed++;
	}
}

bool World::hasVoxel(const glm::ivec3& worldPosition) {
	glm::ivec2 chunkIndex = getChunkIndex(worldPosition);

	if (!chunks.contains(chunkIndex)) {
		return false;
	}

	glm::ivec3 localPosition = getLocalPosition(worldPosition);
	return chunks[chunkIndex]->hasVoxel(localPosition);
}

void World::addVoxel(const glm::ivec3& worldPosition) {
	glm::ivec2 chunkIndex = getChunkIndex(worldPosition);

	if (!chunks.contains(chunkIndex)) {
		return;
	}

	glm::ivec3 localPosition = getLocalPosition(worldPosition);
	chunks[chunkIndex]->setVoxelType(localPosition, VoxelType::STONE);
}

void World::removeVoxel(const glm::ivec3& worldPosition) {
	glm::ivec2 chunkIndex = getChunkIndex(worldPosition);

	if (!chunks.contains(chunkIndex)) {
		return;
	}

	glm::ivec3 localPosition = getLocalPosition(worldPosition);
	chunks[chunkIndex]->setVoxelType(localPosition, VoxelType::STONE);
}

glm::ivec2 World::getChunkIndex(const glm::ivec3& worldPosition) {
	glm::vec2 chunkPos = glm::floor(glm::vec2(worldPosition.x, worldPosition.z) / float(CHUNK_SIZE));
	return glm::ivec2(chunkPos);
}

glm::ivec2 World::getChunkCenterWorld(const glm::ivec2& chunkIndex) {
	glm::ivec2 center = (chunkIndex * CHUNK_SIZE) + int(CHUNK_SIZE * 0.5f);
	return center;
}

glm::ivec3 World::getLocalPosition(const glm::ivec3& worldPosition) {
	glm::ivec3 localPosition;
	localPosition.x = worldPosition.x % CHUNK_SIZE;
	localPosition.z = worldPosition.z % CHUNK_SIZE;
	localPosition.y = worldPosition.y;

	// Handle negatives
	if (localPosition.x < 0) localPosition.x += CHUNK_SIZE;
	if (localPosition.z < 0) localPosition.z += CHUNK_SIZE;

	return localPosition;
}

// Generates a chunk at the given chunk index based on the world's generation type
void World::generateChunk(const glm::ivec2& chunkIndex) {
	Chunk& chunk = *chunks[chunkIndex];

	switch (generationType)
	{
	case GenerationType::Flat:
		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				for (int y = 0; y < 5; y++) {
					if (y < 3) {
						chunk.setVoxelType(glm::ivec3(x, y, z), VoxelType::STONE);
					}
					else {
						chunk.setVoxelType(glm::ivec3(x, y, z), VoxelType::GRASS);
					}
				}
			}
		}

		break;
	case GenerationType::Simple:
		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				glm::vec2 worldPos = glm::vec2((chunkIndex * CHUNK_SIZE) + glm::ivec2(x, z));

				// Noise settings
				const float frequency = 0.003f;
				const float amplitude = 2.5f;
				const int octaves = 5;
				const float lacunarity = 2.5f;
				const float persistence = 0.4f;

				float heightNoise = genNoise2D(worldPos, frequency, amplitude, octaves, lacunarity, persistence);
				float heightValue = heightNoise * MAX_HEIGHT;

				for (int y = 0; y < (int)heightValue; y++) {
					if (y < heightValue - 3) {
						chunk.setVoxelType(glm::ivec3(x, y, z), VoxelType::STONE);
					}
					else {
						chunk.setVoxelType(glm::ivec3(x, y, z), VoxelType::GRASS);
					}
				}
			}
		}
		break;
	case GenerationType::Advanced:
		break;
	default:
		break;
	}

	// Update neighbor meshes
	for (const auto& dir : directions) {
		glm::ivec2 neighborIndex = chunkIndex + dir;

		if (chunks.contains(neighborIndex)) {
			Chunk* neighborChunk = chunks[neighborIndex].get();

			if (!neighborChunk->isMeshValid()) {
				continue;
			}

			neighborChunk->updateMeshBorder(&chunk, -dir);
		}
	}

	chunk.setGenerated();
}

// Generates 2D Perlin noise with multiple octaves and normalizes the result to [0,1]
float World::genNoise2D(const glm::vec2& position, float baseFrequency, float baseAmplitude, int octaves, float lacunarity, float persistence) {
	float total = 0.0f;
	float frequency = baseFrequency;
	float amplitude = baseAmplitude;
	float maxAmplitude = 0.0f;

	for (int i = 0; i < octaves; ++i) {
		float rawNoise = glm::perlin(position * frequency);
		total += rawNoise * amplitude;
		maxAmplitude += amplitude;

		frequency *= lacunarity;
		amplitude *= persistence;
	}

	if (maxAmplitude <= 0.0f) return 0.0f;

	// Normalize from [-maxAmplitude, maxAmplitude] to [-1,1] to [0,1]
	float normalized = (total / maxAmplitude + 1.0f) * 0.5f;
	if (normalized < 0.0f) normalized = 0.0f;
	if (normalized > 1.0f) normalized = 1.0f;
	return normalized;
}
