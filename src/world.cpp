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
	// Create chunk generation threads
	for (int i = 0; i < 4; i++) {
		generationThreads.emplace_back([this, i]() {
			tracy::SetThreadName(("Chunk Generation Thread " + std::to_string(i)).c_str());

			while (generationRunning.load()) {
				ZoneScopedN("Process Chunk");

				glm::ivec2 chunkIndex;
				bool chunkFound = false;

				// Get next chunk to generate
				{
					std::lock_guard<std::mutex> lock(generationQueueMutex);
					std::lock_guard<std::mutex> lock2(processingListMutex);

					if (!generationQueue.empty()) {
						chunkIndex = generationQueue.top().second;

						if (std::find(processingList.begin(), processingList.end(), chunkIndex) == processingList.end()) {
							generationQueue.pop();
							processingList.push_back(chunkIndex);

							chunkFound = true;
						}
					}
				}

				if (!chunkFound) {
					ZoneScopedN("Sleep");
					std::this_thread::sleep_for(std::chrono::milliseconds(15));
					continue;
				}

				// Create chunk if it doesn't exist
				{
					ZoneScopedN("Create");
					std::lock_guard<std::mutex> lock(chunksMutex);

					if (!chunks.contains(chunkIndex)) {
						chunks[chunkIndex] = std::make_shared<Chunk>();
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
}

World::~World() {
	generationRunning.store(false);

	for (auto& thread : generationThreads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
}

void World::draw(const glm::ivec3& worldPosition, const int renderDistance, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material) {
	ZoneScopedN("World Draw");

	glm::ivec2 centerChunkIndex = getChunkIndex(worldPosition);

	for (int x = -renderDistance; x <= renderDistance; x++)
	{
		for (int z = -renderDistance; z <= renderDistance; z++)
		{
			ZoneScopedN("World Draw Chunk");

			glm::ivec2 current = centerChunkIndex + glm::ivec2(x, z);
			std::shared_ptr<Chunk> currentChunk;

			// Skip if chunk doesn't exist
			{
				std::lock_guard<std::mutex> lock(chunksMutex);

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

			// Skip if chunk isn't generated
			if (!currentChunk->isGenerated()) {
				continue;
			}

			// Draw it
			currentChunk->draw(offset, neighbors, view, projection, shader, material);
		}
	}
}

bool World::hasVoxel(const glm::ivec3& worldPosition) {
	glm::ivec2 chunkIndex = getChunkIndex(worldPosition);

	std::shared_ptr<Chunk> chunk;
	{
		std::lock_guard<std::mutex> lock(chunksMutex);
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
		std::lock_guard<std::mutex> lock(chunksMutex);
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
		std::lock_guard<std::mutex> lock(chunksMutex);
		auto it = chunks.find(chunkIndex);
		if (it == chunks.end()) {
			return;
		}
		chunk = it->second;
	}

	glm::ivec3 localPosition = getLocalPosition(worldPosition);
	chunk->setVoxelType(localPosition, VoxelType::STONE);
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

	std::lock_guard<std::mutex> lock(chunksMutex);

	auto it = chunks.find(chunkIndex + directions[0]);
	if (it != chunks.end()) {
		neighbors.nx = it->second;
	}

	it = chunks.find(chunkIndex + directions[1]);
	if (it != chunks.end()) {
		neighbors.px = it->second;
	}

	it = chunks.find(chunkIndex + directions[2]);
	if (it != chunks.end()) {
		neighbors.ny = it->second;
	}

	it = chunks.find(chunkIndex + directions[3]);
	if (it != chunks.end()) {
		neighbors.py = it->second;
	}

	return neighbors;
}

void World::updateGenerationQueue(const glm::ivec3& worldPosition, const int renderDistance) {
	ZoneScopedN("Update Generation Queue");

	std::priority_queue<std::pair<float, glm::ivec2>, std::vector<std::pair<float, glm::ivec2>>, ChunkQueueCompare> tempQueue;
	glm::ivec2 centerChunkIndex = getChunkIndex(worldPosition);
	glm::vec2 worldPos2D(worldPosition.x, worldPosition.z);

	std::lock_guard<std::mutex> lock(generationQueueMutex);
	std::lock_guard<std::mutex> lock2(processingListMutex);

	// Add chunks within render distance to the generation queue
	for (int x = -renderDistance; x <= renderDistance; x++)
	{
		for (int z = -renderDistance; z <= renderDistance; z++)
		{
			glm::ivec2 current = centerChunkIndex + glm::ivec2(x, z);

			// Skip if chunk already exists
			{
				std::lock_guard<std::mutex> lock(chunksMutex);
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
	generationQueue = std::move(tempQueue);
}

// Generates a chunk at the given chunk index based on the world's generation type
void World::generateChunk(const glm::ivec2& chunkIndex) {
	ZoneScopedN("Generate Chunk");

	std::shared_ptr<Chunk> chunk;
	{
		std::lock_guard<std::mutex> lock(chunksMutex);

		auto it = chunks.find(chunkIndex);
		if (it == chunks.end()) {
			throw std::runtime_error("Chunk not found!");
		}

		chunk = it->second;
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

	{
		ZoneScopedN("Neighbors");

		// Update existing neighbor meshes
		for (int i = 0; i < DirectionVectors2D::arr->length(); i++) {
			glm::ivec2 neighborIndex = chunkIndex + DirectionVectors2D::arr[i];

			std::shared_ptr<Chunk> neighborChunk;
			{
				std::lock_guard<std::mutex> lock(chunksMutex);

				auto it = chunks.find(neighborIndex);
				if (it == chunks.end()) {
					continue;
				}

				neighborChunk = it->second;
			}

			if (!neighborChunk->isGenerated() || !neighborChunk->isMeshValid()) {
				continue;
			}

			neighborChunk->updateMeshBorderNew(chunk, DirectionInverted[i]);
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
