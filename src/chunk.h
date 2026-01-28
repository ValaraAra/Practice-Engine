#pragma once

#include "shader.h"
#include "structs.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>
#include <optional>
#include <thread>
#include <set>
#include <array>

class Mesh;
class Chunk;

static constexpr int CHUNK_SIZE = 32;
static constexpr int MAX_HEIGHT = 128;
static constexpr int MAX_VOXELS = CHUNK_SIZE * MAX_HEIGHT * CHUNK_SIZE;

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

class Chunk {
public:
	Chunk(GenerationType generationType, const glm::ivec2& chunkIndex);
	~Chunk();

	void update(const ChunkNeighbors& neighbors);
	void draw(const glm::ivec2 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material);

	bool hasMesh() const {
		return mesh != nullptr;
	}

	bool hasVoxel(const glm::ivec3& position) const;
	void setVoxelType(const glm::ivec3& chunkPosition, const VoxelType type = VoxelType::STONE);
	void clearVoxels();

private:
	std::array<Voxel, MAX_VOXELS> voxels;
	int voxelCount = 0;
	unsigned int version = 0;
	std::mutex voxelFaceMutex;

	std::unique_ptr<Mesh> mesh;
	std::atomic<MeshState> meshState = MeshState::NONE;
	std::atomic<bool> dirty = false;

	std::vector<Face> pendingFaces;
	std::mutex pendingFacesMutex;

	std::optional<std::thread> meshThread;
	std::mutex meshMutex;

	ChunkNeighborVersions neighborVersions;

	struct BorderInfo {
		Axis fixedAxis;
		Axis updateAxis;
		int fixedValue;
		int neighborValue;
		uint8_t faceFlag;
	};

	// PX, NX, PZ, NZ
	static constexpr BorderInfo borderInfoTable[4] = {
		{ Axis::X, Axis::Z, CHUNK_SIZE - 1, 0, VoxelFlags::RIGHT_EXPOSED },
		{ Axis::X, Axis::Z, 0, CHUNK_SIZE - 1, VoxelFlags::LEFT_EXPOSED },
		{ Axis::Z, Axis::X, CHUNK_SIZE - 1, 0, VoxelFlags::FRONT_EXPOSED },
		{ Axis::Z, Axis::X, 0, CHUNK_SIZE - 1, VoxelFlags::BACK_EXPOSED }
	};

	static bool isAdjacentBorderVoxel(const glm::ivec3& position) {
		return position.x == -1 || position.x == CHUNK_SIZE || position.z == -1 || position.z == CHUNK_SIZE;
	};

	static bool isValidPosition(const glm::ivec3& chunkPosition) {
		return (chunkPosition.x >= 0 && chunkPosition.x < CHUNK_SIZE && chunkPosition.y >= 0 && chunkPosition.y < MAX_HEIGHT && chunkPosition.z >= 0 || chunkPosition.z < CHUNK_SIZE);
	};

	glm::ivec3 getChunkPosition(const int chunkIndex) const {
		return { chunkIndex % CHUNK_SIZE, (chunkIndex / CHUNK_SIZE) % MAX_HEIGHT, chunkIndex / (CHUNK_SIZE * MAX_HEIGHT) };
	};

	int getVoxelIndex(const glm::ivec3& chunkPosition) const {
		return chunkPosition.x + chunkPosition.y * CHUNK_SIZE + chunkPosition.z * CHUNK_SIZE * MAX_HEIGHT;
	};

	void startRebuild(const ChunkNeighbors& neighbors, const bool fullRebuild);

	void calculateFaceVisibility(const ChunkNeighbors& neighbors);
	void updateBorderFaceVisibility(const std::shared_ptr<Chunk> neighbor, const Direction2D direction);

	void buildMesh();
};