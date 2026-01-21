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

class Mesh;
class Chunk;

static constexpr int CHUNK_SIZE = 32;
static constexpr int MAX_HEIGHT = 128;
static constexpr int maxVoxels = CHUNK_SIZE * MAX_HEIGHT * CHUNK_SIZE;

struct ChunkNeighbors {
	std::shared_ptr<Chunk> nx;
	std::shared_ptr<Chunk> px;
	std::shared_ptr<Chunk> ny;
	std::shared_ptr<Chunk> py;
};

struct ChunkData {
	Chunk* chunk;
	glm::ivec2 chunkIndex;
};

enum class MeshState {
	NONE,
	DIRTY,
	BUILDING,
	HANDOFF,
	UPLOADING,
	READY,
};

class Chunk {
public:
	Chunk();
	~Chunk();

	void draw(const glm::ivec2 offset, const ChunkNeighbors& neighbors, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material);

	bool isGenerated() const {
		return generated;
	}
	bool setGenerated() {
		return generated = true;
	}

	bool isMeshValid() const {
		return meshState.load() == MeshState::READY; // Just ready? Or also dirty?
	}

	void signalNeighborBorderUpdate(const glm::ivec2& direction) {
		// TBD
	}

	void updateMeshBorder(const Chunk* neighbor, const glm::ivec2& direction);
	void updateMeshBorderNew(const std::shared_ptr<Chunk> neighbor, const Direction direction);

	bool hasVoxel(const glm::ivec3& position) const;
	void setVoxelType(const glm::ivec3& chunkPosition, const VoxelType type = VoxelType::STONE);
	void clearVoxels();

private:
	Voxel voxels[maxVoxels];
	int voxelCount = 0;

	std::optional<std::thread> meshThread;

	std::unique_ptr<Mesh> mesh;
	std::atomic<MeshState> meshState = MeshState::NONE;

	std::vector<Vertex> pendingVertices;
	std::vector<unsigned int> pendingIndices;

	std::mutex voxelFaceMutex;
	std::mutex meshMutex;

	std::queue<std::function<void()>> pendingOperations;
	std::mutex pendingOperationsMutex;

	bool generated = false;

	// Face vertices (right, left, top, bottom, front, back)
	static constexpr glm::vec3 faceVertices[6][4] = {
		{{  0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f,  0.5f }},
		{{ -0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f, -0.5f }},
		{{ -0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }},
		{{ -0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f,  0.5f }, { -0.5f, -0.5f,  0.5f }},
		{{ -0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }},
		{{  0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }},
	};

	// Face normals (right, left, top, bottom, front, back)
	static constexpr glm::vec3 faceNormals[6] = {
		{  1.0f,  0.0f,  0.0f },
		{ -1.0f,  0.0f,  0.0f },
		{  0.0f,  1.0f,  0.0f },
		{  0.0f, -1.0f,  0.0f },
		{  0.0f,  0.0f,  1.0f },
		{  0.0f,  0.0f, -1.0f },
	};

	// Face directions (right, left, top, bottom, front, back)
	static constexpr glm::ivec3 faceDirections[6] = {
		{ 1,  0,  0},
		{-1,  0,  0},
		{ 0,  1,  0},
		{ 0, -1,  0},
		{ 0,  0,  1},
		{ 0,  0, -1},
	};

	struct BorderInfo {
		Axis fixedAxis;
		Axis updateAxis;
		int fixedValue;
		int neighborValue;
		uint8_t faceFlag;
	};

	// NX, PX, NZ, PZ
	static constexpr BorderInfo borderInfoTable[4] = {
		{ Axis::X, Axis::Z, 0, CHUNK_SIZE - 1, VoxelFlags::LEFT_EXPOSED },
		{ Axis::X, Axis::Z, CHUNK_SIZE - 1, 0, VoxelFlags::RIGHT_EXPOSED },
		{ Axis::Z, Axis::X, 0, CHUNK_SIZE - 1, VoxelFlags::BACK_EXPOSED },
		{ Axis::Z, Axis::X, CHUNK_SIZE - 1, 0, VoxelFlags::FRONT_EXPOSED }
	};


	static bool isBorderVoxel(const glm::ivec3& position);
	static bool isAdjacentBorderVoxel(const glm::ivec3& position);
	static bool isValidPosition(const glm::ivec3& chunkPosition);

	int getVoxelIndex(const glm::ivec3& chunkPosition) const;

	void calculateFaceVisibility(const ChunkNeighbors& neighbors);

	void updateMesh(const ChunkNeighbors& neighbors);
	void buildMesh();

	void performSetVoxelType(const glm::ivec3& chunkPosition, const VoxelType type);
	void performClearVoxels();
};