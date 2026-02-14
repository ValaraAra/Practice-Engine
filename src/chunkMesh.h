#pragma once

#include "shader.h"
#include "structs.h"
#include "chunk.h"
#include "primitives/mesh.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_set>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include <queue>
#include <optional>
#include <thread>
#include <set>
#include <bitset>
#include <map>

struct ChunkNeighbors {
	std::shared_ptr<Chunk> px;
	std::shared_ptr<Chunk> nx;
	std::shared_ptr<Chunk> pz;
	std::shared_ptr<Chunk> nz;
};

enum class MeshState {
	NONE,
	BUILDING,
	HANDOFF,
	UPLOADING,
	READY,
};

class ChunkMesh {
public:
	ChunkMesh();
	~ChunkMesh();

	void update();
	void draw(const glm::ivec2 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material);
	void build(const std::shared_ptr<Chunk> chunk, const ChunkNeighbors& neighbors);

	bool isValid() const {
		return mesh != nullptr;
	}

private:
	std::atomic<MeshState> meshState = MeshState::NONE;
	std::unique_ptr<Mesh> mesh;

	std::array<uint32_t, CHUNK_SIZE * MAX_HEIGHT> occupancyMasks;

	std::vector<Face> faces;
	std::mutex faceMutex;

	static bool isAdjacentBorderVoxel(const glm::ivec3& position) {
		return position.x == -1 || position.x == CHUNK_SIZE || position.z == -1 || position.z == CHUNK_SIZE;
	};

	static bool isValidPosition(const glm::ivec3& chunkPosition) {
		return (chunkPosition.x >= 0 && chunkPosition.x < CHUNK_SIZE && chunkPosition.y >= 0 && chunkPosition.y < MAX_HEIGHT && chunkPosition.z >= 0 && chunkPosition.z < CHUNK_SIZE);
	};

	glm::ivec3 getChunkPosition(const int chunkIndex) const {
		return { chunkIndex % CHUNK_SIZE, (chunkIndex / CHUNK_SIZE) % MAX_HEIGHT, chunkIndex / (CHUNK_SIZE * MAX_HEIGHT) };
	};

	int getVoxelIndex(const glm::ivec3& chunkPosition) const {
		return chunkPosition.x + chunkPosition.y * CHUNK_SIZE + chunkPosition.z * CHUNK_SIZE * MAX_HEIGHT;
	};

	void buildMasks(const std::array<Voxel, MAX_VOXELS>& chunkVoxels);
	void addFace(const glm::ivec3 pos, const VoxelType type, const uint8_t face);
	void emitFaces(uint32_t mask, int y, int z, uint8_t direction, const std::array<Voxel, MAX_VOXELS>& voxels);
	void buildFaces(const std::array<Voxel, MAX_VOXELS>& chunkVoxels, const ChunkNeighbors& neighbors);
};