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

	glm::ivec2 centerChunkPos = getChunkIndex(worldPosition);

	std::vector<ChunkDrawingInfo> chunksToDraw;

	{
		ZoneScopedN("Process Chunks");

		// Process chunks in render distance
		for (int x = -renderDistance; x <= renderDistance; x++)
		{
			for (int z = -renderDistance; z <= renderDistance; z++)
			{
				ZoneScopedN("Process Chunk");

				glm::ivec2 currentChunkPos = centerChunkPos + glm::ivec2(x, z);
				std::shared_ptr<ChunkMesh> currentMesh;

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
				currentMesh->update(getChunkNeighbors(currentChunkPos));

				// Skip if mesh isn't valid
				if (!currentMesh->isValid()) {
					continue;
				}

				// Add to draw list
				chunksToDraw.push_back({ currentMesh, currentChunkPos * CHUNK_SIZE, distanceToChunkCenterWorld });
			}
		}
	}

	// Draw chunks
	{
		ZoneScopedN("Draw Chunks");

		// Maybe sort by distance here

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

					// Mesh or remesh depending on case
					if (!mesh->isValid() || chunk->isDirty()) {
						chunk->clearDirty();
						mesh->remesh(chunk, getChunkNeighbors(chunkIndex));
					}
					else if (mesh->bordersNeedUpdate()) {
						mesh->remesh(chunk, getChunkNeighbors(chunkIndex));
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

			// Skip if mesh exists and doesn't need an update
			{
				std::shared_lock meshesLock(meshesMutex);
				std::shared_ptr<ChunkMesh> mesh;

				auto it = meshes.find(current);
				if (it != meshes.end()) {
					mesh = it->second;

					// Checks if chunk is dirty or if borders need update
					if (!chunk->isDirty() && !mesh->bordersNeedUpdate()) {
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

	std::shared_ptr<Chunk> chunk;
	{
		ZoneScopedN("Create");
		std::lock_guard lock(chunksMutex);

		auto it = chunks.find(chunkIndex);
		if (it == chunks.end()) {
			chunk = std::make_shared<Chunk>();
			chunks[chunkIndex] = chunk;
		}
		else {
			chunk = it->second;
		}
	}

	{
		ZoneScopedN("Generate");

		switch (generationType)
		{
		case GenerationType::Flat:
			for (int x = 0; x < CHUNK_SIZE; x++) {
				for (int z = 0; z < CHUNK_SIZE; z++) {
					for (int y = 0; y < 5; y++) {
						if (y < 3) {
							chunk->setVoxelType(glm::ivec3(x, y, z), VoxelType::STONE);
						}
						else {
							chunk->setVoxelType(glm::ivec3(x, y, z), VoxelType::GRASS);
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
							chunk->setVoxelType(glm::ivec3(x, y, z), VoxelType::STONE);
						}
						else {
							chunk->setVoxelType(glm::ivec3(x, y, z), VoxelType::GRASS);
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
	}

	chunk->setGenerated();
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
