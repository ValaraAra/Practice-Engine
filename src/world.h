#pragma once

#include "shader.h"
#include "chunk.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <memory>
#include <queue>

enum class GenerationType {
	Flat,
	Simple,
	Advanced,
};

class World {
public:
	World(GenerationType generationType = GenerationType::Flat);
	~World();

	void draw(const glm::ivec3& worldPosition, const int renderDistance, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material);

	void processGenerationQueue(int maxChunksPerIteration = 1);

	bool hasVoxel(const glm::ivec3& position);
	void addVoxel(const glm::ivec3& position);
	void removeVoxel(const glm::ivec3& position);

	glm::ivec3 getChunkIndex(const glm::ivec3& worldPosition);
	glm::ivec3 getChunkCenterWorld(const glm::ivec3& chunkIndex);
	glm::ivec3 getLocalPosition(const glm::ivec3& worldPosition);
	Chunk& getOrCreateChunk(const glm::ivec3& worldPosition);

	GenerationType generationType = GenerationType::Flat;

private:
	std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ivec3Hasher> chunks;
	std::queue<glm::ivec3> generationQueue;

	void generateChunk(const glm::ivec3& chunkIndex);
};