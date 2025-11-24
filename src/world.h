#pragma once

#include "shader.h"
#include "chunk.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <memory>
#include <queue>
#include <unordered_set>
#include <utility>

struct ChunkQueueCompare {
	bool operator()(const std::pair<float, glm::ivec2>& a, const std::pair<float, glm::ivec2>& b) const noexcept {
		return a.first < b.first;
	}
};

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

	glm::ivec2 getChunkIndex(const glm::ivec3& worldPosition);
	glm::ivec2 getChunkCenterWorld(const glm::ivec2& chunkIndex);
	glm::ivec3 getLocalPosition(const glm::ivec3& worldPosition);

	GenerationType generationType = GenerationType::Flat;

private:
	std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, ivec2Hasher> chunks;
	std::priority_queue<std::pair<float, glm::ivec2>, std::vector<std::pair<float, glm::ivec2>>, ChunkQueueCompare> generationQueue;
	std::unordered_set<glm::ivec2, ivec2Hasher> chunksInQueue;

	float genNoise2D(const glm::vec2& position, float baseFrequency, float baseAmplitude, int octaves, float lacunarity, float persistence);

	void generateChunk(const glm::ivec2& chunkIndex);
};