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
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

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

	ChunkNeighbors getChunkNeighbors(glm::ivec2 chunkIndex);

	void updateGenerationQueue(const glm::ivec3& worldPosition, const int renderDistance);

	bool hasVoxel(const glm::ivec3& position);
	void addVoxel(const glm::ivec3& position);
	void removeVoxel(const glm::ivec3& position);

	int getChunkCount();
	glm::ivec2 getChunkIndex(const glm::ivec3& worldPosition);
	glm::ivec2 getChunkCenterWorld(const glm::ivec2& chunkIndex);
	glm::ivec3 getLocalPosition(const glm::ivec3& worldPosition);

	GenerationType generationType = GenerationType::Flat;

private:
	std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, ivec2Hasher> chunks;
	std::priority_queue<std::pair<float, glm::ivec2>, std::vector<std::pair<float, glm::ivec2>>, ChunkQueueCompare> generationQueue;

	std::shared_mutex chunksMutex;
	std::mutex generationQueueMutex;
	std::mutex processingListMutex;
	std::condition_variable generationCondition;
	std::atomic<bool> stopGeneration = false;

	std::vector<glm::ivec2> processingList;
	std::vector<std::thread> generationThreads;

	static constexpr glm::ivec2 directions[4] = {
		{ -1, 0 },
		{ 1, 0 },
		{ 0, -1 },
		{ 0, 1 }
	};

	void resetGenerationQueue() {
		generationQueue = std::priority_queue<std::pair<float, glm::ivec2>, std::vector<std::pair<float, glm::ivec2>>, ChunkQueueCompare>();
	}

	void generateChunk(const glm::ivec2& chunkIndex);
	float genNoise2D(const glm::vec2& position, float baseFrequency, float baseAmplitude, int octaves, float lacunarity, float persistence);
};