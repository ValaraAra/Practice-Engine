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
#include <chrono>
#include <thread>
#include <tracy/Tracy.hpp>

World::World(GenerationType generationType) : generationType(generationType) {

}

World::~World() {
	stopGeneration.store(true);
	generationCondition.notify_all();

	for (auto& thread : generationThreads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

struct ChunkDrawingInfo {
	std::shared_ptr<Chunk> chunk;
	glm::ivec2 offset;
	float distance;
};

void World::draw(const glm::ivec3& worldPosition, const int renderDistance, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material, const bool wireframe) {
	ZoneScopedN("World Draw");

	// Set polygon mode to line if wireframe mode enabled
	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
	}

	glm::ivec2 centerChunkIndex = getChunkIndex(worldPosition);

	std::vector<ChunkDrawingInfo> chunksToDraw;

	{
		ZoneScopedN("Process Chunks");

		// Process chunks in render distance
		for (int x = -renderDistance; x <= renderDistance; x++)
		{
			for (int z = -renderDistance; z <= renderDistance; z++)
			{
				ZoneScopedN("Process Chunk");

				glm::ivec2 current = centerChunkIndex + glm::ivec2(x, z);
				std::shared_ptr<Chunk> currentChunk;

				// Skip if chunk doesn't exist
				{
					std::shared_lock lock(chunksMutex);

					auto it = chunks.find(current);
					if (it == chunks.end()) {
						continue;
					}

					currentChunk = it->second;
				}

				// Calculate max distance and distance to chunk center in world space
				glm::vec2 chunkCenterWorld = getChunkCenterWorld(current);
				float distanceToChunkCenterWorld = glm::length(chunkCenterWorld - glm::vec2(worldPosition.x, worldPosition.z));
				float maxDistanceWorld = float(renderDistance) * float(CHUNK_SIZE);

				// Skip if outside render distance
				if (distanceToChunkCenterWorld > maxDistanceWorld) {
					continue;
				}

				glm::ivec2 offset = current * CHUNK_SIZE;
				ChunkNeighbors neighbors = getChunkNeighbors(current);

				// Update it
				currentChunk->update(neighbors);

				// Add to draw list
				chunksToDraw.push_back({ currentChunk, offset, distanceToChunkCenterWorld });
			}
		}
	}

	// Draw chunks
	{
		ZoneScopedN("Draw Chunks");

		// Maybe sort by distance here

		for (const ChunkDrawingInfo& chunkInfo : chunksToDraw) {
			chunkInfo.chunk->draw(chunkInfo.offset, view, projection, shader, material);
		}
	}

	// Reset polygon mode if wireframe mode enabled
	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
	}
}

bool World::hasVoxel(const glm::ivec3& worldPosition) {
	glm::ivec2 chunkIndex = getChunkIndex(worldPosition);

	std::shared_ptr<Chunk> chunk;
	{
		std::shared_lock lock(chunksMutex);
		auto it = chunks.find(chunkIndex);
		if (it == chunks.end()) {
			return false;
		}
		chunk = it->second;
	}

	glm::ivec3 localPosition = getLocalPosition(worldPosition);
	return chunk->hasVoxel(localPosition);
}

void World::addVoxel(const glm::ivec3& worldPosition) {
	glm::ivec2 chunkIndex = getChunkIndex(worldPosition);

	std::shared_ptr<Chunk> chunk;
	{
		std::shared_lock lock(chunksMutex);
		auto it = chunks.find(chunkIndex);
		if (it == chunks.end()) {
			return;
		}
		chunk = it->second;
	}

	glm::ivec3 localPosition = getLocalPosition(worldPosition);
	chunk->setVoxelType(localPosition, VoxelType::STONE);
}

void World::removeVoxel(const glm::ivec3& worldPosition) {
	glm::ivec2 chunkIndex = getChunkIndex(worldPosition);

	std::shared_ptr<Chunk> chunk;
	{
		std::shared_lock lock(chunksMutex);
		auto it = chunks.find(chunkIndex);
		if (it == chunks.end()) {
			return;
		}
		chunk = it->second;
	}

	glm::ivec3 localPosition = getLocalPosition(worldPosition);
	chunk->setVoxelType(localPosition, VoxelType::EMPTY);
}

int World::getChunkCount()
{
	std::shared_lock lock(chunksMutex);
	return static_cast<int>(chunks.size());
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
	glm::ivec3 localPosition(0);
	localPosition.x = worldPosition.x % CHUNK_SIZE;
	localPosition.z = worldPosition.z % CHUNK_SIZE;
	localPosition.y = worldPosition.y;

	// Handle negatives
	if (localPosition.x < 0) localPosition.x += CHUNK_SIZE;
	if (localPosition.z < 0) localPosition.z += CHUNK_SIZE;

	return localPosition;
}

ChunkNeighbors World::getChunkNeighbors(glm::ivec2 chunkIndex) {
	ChunkNeighbors neighbors = {};

	std::shared_lock lock(chunksMutex);

	auto it = chunks.find(chunkIndex + DirectionVectors2D::PX);
	if (it != chunks.end()) {
		neighbors.px = it->second;
	}

	it = chunks.find(chunkIndex + DirectionVectors2D::NX);
	if (it != chunks.end()) {
		neighbors.nx = it->second;
	}

	it = chunks.find(chunkIndex + DirectionVectors2D::PZ);
	if (it != chunks.end()) {
		neighbors.pz = it->second;
	}

	it = chunks.find(chunkIndex + DirectionVectors2D::NZ);
	if (it != chunks.end()) {
		neighbors.nz = it->second;
	}

	return neighbors;
}

void World::startGenerationThreads() {
	std::call_once(generationThreadsStarted, [this]() {
		// Create chunk generation threads
		for (int i = 0; i < 4; i++) {
			generationThreads.emplace_back([this, i]() {
				tracy::SetThreadName(("Chunk Generation Thread " + std::to_string(i)).c_str());

				while (!stopGeneration.load()) {
					ZoneScopedN("Process Chunk");

					glm::ivec2 chunkIndex;

					// Get next chunk to generate
					{
						std::unique_lock<std::mutex> lock(generationQueueMutex);
						generationCondition.wait(lock, [this] { return stopGeneration.load() || !generationQueue.empty(); });

						if (stopGeneration.load()) {
							break;
						}

						std::lock_guard<std::mutex> lock2(processingListMutex);

						chunkIndex = generationQueue.top().second;

						if (std::find(processingList.begin(), processingList.end(), chunkIndex) == processingList.end()) {
							generationQueue.pop();
							processingList.push_back(chunkIndex);
						}
						else {
							continue;
						}
					}

					// Generate chunk
					generateChunk(chunkIndex);

					// Remove from processing list
					std::lock_guard<std::mutex> lock(processingListMutex);
					processingList.erase(std::remove(processingList.begin(), processingList.end(), chunkIndex), processingList.end());
				}
			});
		}
	});
}

void World::updateGenerationQueue(const glm::ivec3& worldPosition, const int renderDistance) {
	ZoneScopedN("Update Generation Queue");

	startGenerationThreads();

	std::priority_queue<std::pair<float, glm::ivec2>, std::vector<std::pair<float, glm::ivec2>>, ChunkQueueCompare> tempQueue;

	glm::ivec2 centerChunkIndex = getChunkIndex(worldPosition);
	glm::vec2 worldPos2D(worldPosition.x, worldPosition.z);

	std::lock_guard<std::mutex> processingLock(processingListMutex);

	// Add chunks within render distance to the generation queue
	for (int x = -renderDistance; x <= renderDistance; x++)
	{
		for (int z = -renderDistance; z <= renderDistance; z++)
		{
			glm::ivec2 current = centerChunkIndex + glm::ivec2(x, z);

			// Skip if chunk already exists (need to update this, should only skip generated chunks)
			{
				std::shared_lock chunksLock(chunksMutex);
				if (chunks.contains(current)) {
					continue;
				}
			}

			// Skip if chunk already being processed
			if (std::find(processingList.begin(), processingList.end(), current) != processingList.end()) {
				continue;
			}

			// Calculate distance to chunk center in world space
			glm::vec2 chunkCenterWorld = getChunkCenterWorld(current);
			float distanceToChunkCenterWorld = glm::length(chunkCenterWorld - worldPos2D);

			tempQueue.push(std::make_pair(-distanceToChunkCenterWorld, current));
		}
	}

	// Swap the old gen queue with the new one
	std::lock_guard<std::mutex> genLock(generationQueueMutex);
	generationQueue = std::move(tempQueue);

	// Notify generation threads
	generationCondition.notify_all();
}

// Generates a chunk at the given chunk index based on the world's generation type
void World::generateChunk(const glm::ivec2& chunkIndex) {
	ZoneScopedN("Generate Chunk");

	// Check if a chunk already exists at index
	{
		std::lock_guard lock(chunksMutex);

		if (chunks.contains(chunkIndex)) {
			std::cerr << "Tried generating chunk at " << chunkIndex.x << ", " << chunkIndex.y << " but it already exists!" << std::endl;
			return;
		}
	}

	std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(generationType, chunkIndex);

	{
		ZoneScopedN("Insert");
		std::lock_guard lock(chunksMutex);

		chunks.insert({ chunkIndex, chunk });
	}

}
