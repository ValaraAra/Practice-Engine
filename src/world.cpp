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
	// Handle generation threads
	stopGeneration.store(true);
	generationCondition.notify_all();

	for (auto& thread : generationThreads) {
		if (thread.joinable()) {
			thread.join();
		}
	}

	// Handle meshing threads
	stopMeshing.store(true);
	meshingCondition.notify_all();

	for (auto& thread : meshingThreads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

struct ChunkDrawingInfo {
	std::shared_ptr<ChunkMesh> mesh;
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

	// Extract frustrum planes (for culling)
	const glm::mat4 vpt = glm::transpose(projection * view);
	const std::vector<glm::vec4> frustumPlanes = {
		(vpt[3] + vpt[0]),
		(vpt[3] - vpt[0]),
		(vpt[3] + vpt[1]),
		(vpt[3] - vpt[1]),
		(vpt[3] + vpt[2]),
		(vpt[3] - vpt[2])
	};

	std::vector<ChunkDrawingInfo> chunksToDraw;
	glm::ivec2 centerChunkIndex = getChunkIndex(worldPosition);

	{
		ZoneScopedN("Process Chunks");

		// Process chunks in render distance (really need to merge the branch that splits this up, and only need to do some parts when moving to a new chunk)
		for (int x = -renderDistance; x <= renderDistance; x++)
		{
			for (int z = -renderDistance; z <= renderDistance; z++)
			{
				ZoneScopedN("Process Chunk");

				glm::ivec2 currentChunkPos = centerChunkPos + glm::ivec2(x, z);
				std::shared_ptr<ChunkMesh> currentMesh;
				
				// Skip if chunk isn't visible
				if (!frustrumAABBVisibility(currentChunkPos, frustumPlanes)) {
					continue;
				}

				// Skip if mesh doesn't exist
				{
					std::shared_lock lock(meshesMutex);

					auto it = meshes.find(currentChunkPos);
					if (it == meshes.end()) {
						continue;
					}

					currentMesh = it->second;
				}

				// Calculate max distance and distance to chunk center in world space
				glm::vec2 chunkCenterWorld = getChunkCenterWorld(currentChunkPos);
				float distanceToChunkCenterWorld = glm::length(chunkCenterWorld - glm::vec2(worldPosition.x, worldPosition.z));
				float maxDistanceWorld = float(renderDistance) * float(CHUNK_SIZE);

				// Skip if outside render distance
				if (distanceToChunkCenterWorld > maxDistanceWorld) {
					continue;
				}

				// Update mesh
				currentMesh->update();

				// Skip if mesh isn't valid
				if (!currentMesh->isValid()) {
					continue;
				}

				// Add to draw list
				chunksToDraw.push_back({ currentMesh, currentChunkPos * CHUNK_SIZE, distanceToChunkCenterWorld });
			}
		}
	}

	renderedChunkCount = chunksToDraw.size();

	// Draw chunks
	{
		ZoneScopedN("Draw Chunks");

		for (const ChunkDrawingInfo& chunkInfo : chunksToDraw) {
			chunkInfo.mesh->draw(chunkInfo.offset, view, projection, shader, material);
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

int World::getChunkCount() {
	std::shared_lock lock(chunksMutex);
	return static_cast<int>(chunks.size());
}

int World::getRenderedChunkCount() {
	return static_cast<int>(renderedChunkCount);
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
	std::call_once(startedGenerationThreads, [this]() {
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

						std::lock_guard<std::mutex> lock2(generationProcessingListMutex);

						chunkIndex = generationQueue.top().second;

						if (std::find(generationProcessingList.begin(), generationProcessingList.end(), chunkIndex) == generationProcessingList.end()) {
							generationQueue.pop();
							generationProcessingList.push_back(chunkIndex);
						}
						else {
							continue;
						}
					}

					// Generate chunk
					generateChunk(chunkIndex);

					// Remove from processing list
					std::lock_guard<std::mutex> lock(generationProcessingListMutex);
					generationProcessingList.erase(std::remove(generationProcessingList.begin(), generationProcessingList.end(), chunkIndex), generationProcessingList.end());
				}
			});
		}
	});
}

void World::startMeshingThreads() {
	std::call_once(startedMeshingThreads, [this]() {
		// Create chunk generation threads
		for (int i = 0; i < 4; i++) {
			meshingThreads.emplace_back([this, i]() {
				tracy::SetThreadName(("Meshing Thread " + std::to_string(i)).c_str());

				while (!stopMeshing.load()) {
					ZoneScopedN("Mesh Chunk");

					glm::ivec2 chunkIndex;

					// Get next chunk to mesh
					{
						std::unique_lock<std::mutex> lock(meshingQueueMutex);
						meshingCondition.wait(lock, [this] { return stopMeshing.load() || !meshingQueue.empty(); });

						if (stopMeshing.load()) {
							break;
						}

						std::lock_guard<std::mutex> lock2(meshingProcessingListMutex);

						chunkIndex = meshingQueue.top().second;

						if (std::find(meshingProcessingList.begin(), meshingProcessingList.end(), chunkIndex) == meshingProcessingList.end()) {
							meshingQueue.pop();
							meshingProcessingList.push_back(chunkIndex);
						}
						else {
							continue;
						}
					}

					// Get chunk, queue should guarantee it exists
					std::shared_ptr<Chunk> chunk;
					{
						std::shared_lock lock(chunksMutex);
						auto it = chunks.find(chunkIndex);
						if (it == chunks.end()) {
							std::cerr << "Chunk doesn't exist!" << std::endl;
							continue;
						}
						chunk = it->second;
					}

					// Get existing mesh or make a new one
					std::shared_ptr<ChunkMesh> mesh;
					{
						std::unique_lock lock(meshesMutex);
						auto it = meshes.find(chunkIndex);
						if (it != meshes.end()) {
							mesh = it->second;
						}
						else {
							mesh = std::make_shared<ChunkMesh>();
							meshes[chunkIndex] = mesh;
						}
					}

					// Mesh
					if (!mesh->isValid() || chunk->isDirty()) {
						chunk->clearDirty();
						mesh->build(chunk, getChunkNeighbors(chunkIndex));
					}

					// Remove from processing list
					std::lock_guard<std::mutex> lock(meshingProcessingListMutex);
					meshingProcessingList.erase(std::remove(meshingProcessingList.begin(), meshingProcessingList.end(), chunkIndex), meshingProcessingList.end());
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

	std::lock_guard<std::mutex> processingLock(generationProcessingListMutex);

	// Add chunks within render distance to the generation queue
	for (int x = -renderDistance; x <= renderDistance; x++)
	{
		for (int z = -renderDistance; z <= renderDistance; z++)
		{
			glm::ivec2 current = centerChunkIndex + glm::ivec2(x, z);

			// Skip if chunk has already been generated
			{
				std::shared_lock chunksLock(chunksMutex);
				auto it = chunks.find(current);
				if (it != chunks.end() && it->second->isGenerated()) {
					continue;
				}
			}

			// Skip if chunk already being processed
			if (std::find(generationProcessingList.begin(), generationProcessingList.end(), current) != generationProcessingList.end()) {
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

void World::updateMeshingQueue(const glm::ivec3& worldPosition, const int renderDistance) {
	ZoneScopedN("Update Meshing Queue");

	startMeshingThreads();

	std::priority_queue<std::pair<float, glm::ivec2>, std::vector<std::pair<float, glm::ivec2>>, ChunkQueueCompare> tempQueue;

	glm::ivec2 centerChunkIndex = getChunkIndex(worldPosition);
	glm::vec2 worldPos2D(worldPosition.x, worldPosition.z);

	std::lock_guard<std::mutex> processingLock(meshingProcessingListMutex);

	// Add chunks within render distance to the meshing queue if they need an update
	for (int x = -renderDistance; x <= renderDistance; x++)
	{
		for (int z = -renderDistance; z <= renderDistance; z++)
		{
			glm::ivec2 current = centerChunkIndex + glm::ivec2(x, z);
			std::shared_ptr<Chunk> chunk;

			// Skip if chunk hasn't been generated yet
			{
				std::shared_lock chunksLock(chunksMutex);

				auto it = chunks.find(current);
				if (it == chunks.end() || !it->second->isGenerated()) {
					continue;
				}
				chunk = it->second;
			}

			// Skip if neighbors haven't been generated yet
			{
				ChunkNeighbors neighbors = getChunkNeighbors(current);

				if (!neighbors.px || !neighbors.px->isGenerated() ||
					!neighbors.nx || !neighbors.nx->isGenerated() ||
					!neighbors.pz || !neighbors.pz->isGenerated() ||
					!neighbors.nz || !neighbors.nz->isGenerated()) {
					continue;
				}
			}

			// Skip if mesh exists and doesn't need an update
			{
				std::shared_lock meshesLock(meshesMutex);

				auto it = meshes.find(current);
				if (it != meshes.end()) {
					if (!chunk->isDirty()) {
						continue;
					}
				}
			}

			// Skip if mesh is already being processed
			if (std::find(meshingProcessingList.begin(), meshingProcessingList.end(), current) != meshingProcessingList.end()) {
				continue;
			}

			// Calculate distance to chunk center in world space
			glm::vec2 chunkCenterWorld = getChunkCenterWorld(current);
			float distanceToChunkCenterWorld = glm::length(chunkCenterWorld - worldPos2D);

			tempQueue.push(std::make_pair(-distanceToChunkCenterWorld, current));
		}
	}

	// Swap the old mesh queue with the new one
	std::lock_guard<std::mutex> meshLock(meshingQueueMutex);
	meshingQueue = std::move(tempQueue);

	// Notify generation threads
	meshingCondition.notify_all();
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

bool World::frustrumAABBVisibility(const glm::ivec2& chunkIndex, const std::vector<glm::vec4>& frustrumPlanes) {
	glm::vec4 vmin = glm::vec4(chunkIndex.x * CHUNK_SIZE, 0, chunkIndex.y * CHUNK_SIZE, 1.0f);
	glm::vec4 vmax = vmin + glm::vec4(CHUNK_SIZE, MAX_HEIGHT, CHUNK_SIZE, 0.0f);

	for (const glm::vec4& plane : frustrumPlanes) {
		if ((glm::dot(plane, vmin) < 0.0f) &&
			(glm::dot(plane, glm::vec4(vmax.x, vmin.y, vmin.z, 1.0f)) < 0.0f) &&
			(glm::dot(plane, glm::vec4(vmin.x, vmax.y, vmin.z, 1.0f)) < 0.0f) &&
			(glm::dot(plane, glm::vec4(vmax.x, vmax.y, vmin.z, 1.0f)) < 0.0f) &&
			(glm::dot(plane, glm::vec4(vmin.x, vmin.y, vmax.z, 1.0f)) < 0.0f) &&
			(glm::dot(plane, glm::vec4(vmax.x, vmin.y, vmax.z, 1.0f)) < 0.0f) &&
			(glm::dot(plane, glm::vec4(vmin.x, vmax.y, vmax.z, 1.0f)) < 0.0f) &&
			(glm::dot(plane, vmax) < 0.0f))
		{
			return false;
		}
	}

	return true;
}
