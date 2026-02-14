#pragma once

#include "shader.h"
#include "chunk.h"
#include "chunkMesh.h"
#include "structs.h"
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

class World {
public:
	World(GenerationType generationType = GenerationType::Flat);
	~World();

	void draw(const glm::ivec3& worldPosition, const int renderDistance, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material, const bool wireframe = false);

	ChunkNeighbors getChunkNeighbors(glm::ivec2 chunkIndex);

	void updateGenerationQueue(const glm::ivec3& worldPosition, const int renderDistance);
	void updateMeshingQueue(const glm::ivec3& worldPosition, const int renderDistance);

	bool hasVoxel(const glm::ivec3& position);
	void addVoxel(const glm::ivec3& position);
	void removeVoxel(const glm::ivec3& position);

	int getChunkCount();
	int getRenderedChunkCount();
	glm::ivec2 getChunkIndex(const glm::ivec3& worldPosition);
	glm::ivec2 getChunkCenterWorld(const glm::ivec2& chunkIndex);
	glm::ivec3 getLocalPosition(const glm::ivec3& worldPosition);

	GenerationType generationType = GenerationType::Flat;

private:
	void startMeshingThreads();
	void startGenerationThreads();

	// Chunks
	std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, ivec2Hasher> chunks;
	std::shared_mutex chunksMutex;

	// Meshes
	std::unordered_map<glm::ivec2, std::shared_ptr<ChunkMesh>, ivec2Hasher> meshes;
	std::shared_mutex meshesMutex;

	// Generation
	std::priority_queue<std::pair<float, glm::ivec2>, std::vector<std::pair<float, glm::ivec2>>, ChunkQueueCompare> generationQueue;
	std::mutex generationQueueMutex;

	std::vector<glm::ivec2> generationProcessingList;
	std::mutex generationProcessingListMutex;

	std::vector<std::thread> generationThreads;
	std::condition_variable generationCondition;
	std::atomic<bool> stopGeneration = false;
	std::once_flag startedGenerationThreads;

	// Meshing
	std::priority_queue<std::pair<float, glm::ivec2>, std::vector<std::pair<float, glm::ivec2>>, ChunkQueueCompare> meshingQueue;
	std::mutex meshingQueueMutex;

	std::vector<glm::ivec2> meshingProcessingList;
	std::mutex meshingProcessingListMutex;

	std::vector<std::thread> meshingThreads;
	std::condition_variable meshingCondition;
	std::atomic<bool> stopMeshing = false;
	std::once_flag startedMeshingThreads;
	
	size_t renderedChunkCount = 0;

	void resetGenerationQueue() {
		generationQueue = std::priority_queue<std::pair<float, glm::ivec2>, std::vector<std::pair<float, glm::ivec2>>, ChunkQueueCompare>();
	}

	void resetMeshingQueue() {
		meshingQueue = std::priority_queue<std::pair<float, glm::ivec2>, std::vector<std::pair<float, glm::ivec2>>, ChunkQueueCompare>();
	}

	void generateChunk(const glm::ivec2& chunkIndex);

	static bool frustrumAABBVisibility(const glm::ivec2& chunkIndex, const std::vector<glm::vec4>& frustrumPlanes);
};