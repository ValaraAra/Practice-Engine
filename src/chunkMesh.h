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

struct ChunkNeighborVersions {
	unsigned int px = 0;
	unsigned int nx = 0;
	unsigned int pz = 0;
	unsigned int nz = 0;
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

	void update(const ChunkNeighbors& neighbors);
	void draw(const glm::ivec2 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material);
	void clear();

	void remesh(const std::shared_ptr<Chunk> chunk, const ChunkNeighbors& neighbors);
	void remeshBorders(const std::shared_ptr<Chunk> chunk, const ChunkNeighbors& neighbors);

	bool isValid() const {
		return meshValid.load();
	}

	bool bordersNeedUpdate() const {
		std::shared_lock lock(neighborsMutex);
		return neighborsUpdated.any();
	};

private:
	std::atomic<MeshState> meshState = MeshState::NONE;

	std::unique_ptr<Mesh> mesh;
	std::mutex meshMutex;
	std::atomic<bool> meshValid = false;

	std::map<glm::ivec4, Face, ivec4Comparator> faces;
	std::mutex faceMutex;

	ChunkNeighborVersions neighborVersions;
	std::bitset<6> neighborsUpdated;
	mutable std::shared_mutex neighborsMutex;

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

	void rebuildFaces(const std::array<Voxel, MAX_VOXELS>&& chunkVoxels, const std::array<std::shared_ptr<std::array<Voxel, CHUNK_SIZE * MAX_HEIGHT>>, 4> neighborBorderVoxels);
	void rebuildBorderFaces(const std::array<Voxel, CHUNK_SIZE * MAX_HEIGHT>&& chunkBorderVoxels, const std::array<Voxel, CHUNK_SIZE * MAX_HEIGHT>&& neighborBorderVoxels, const Direction2D direction);
};