#pragma once

#include "shader.h"
#include "structs.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_set>
#include <memory>

class Mesh;
class Chunk;

static const int CHUNK_SIZE = 32;
static const int MAX_HEIGHT = 128;

struct ChunkNeighbors {
	Chunk* nx = nullptr;
	Chunk* px = nullptr;
	Chunk* ny = nullptr;
	Chunk* py = nullptr;
};

struct ChunkData {
	Chunk* chunk;
	glm::ivec2 chunkIndex;
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
		return !rebuildMesh;
	}
	void updateMesh(const Chunk* neighbor, const glm::ivec2& direction);

	void updateMeshBorder(const Chunk* neighbor, const glm::ivec2& direction);

	bool hasVoxel(const glm::ivec3& position) const;
	void addVoxel(const glm::ivec3& chunkPosition, const VoxelType type = VoxelType::STONE);
	void setVoxelType(const glm::ivec3& chunkPosition, const VoxelType type);
	void removeVoxel(const glm::ivec3& position);
	void clearVoxels();

	void calculateFaceVisibility(const ChunkNeighbors& neighbors);

private:
	static const int maxVoxels = CHUNK_SIZE * MAX_HEIGHT * CHUNK_SIZE;
	Voxel voxels[maxVoxels];
	int voxelCount = 0;

	bool generated = false;

	// Face vertices (right, left, top, bottom, front, back)
	const glm::vec3 faceVertices[6][4] = {
		{{  0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f,  0.5f }},
		{{ -0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f, -0.5f }},
		{{ -0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }},
		{{ -0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f,  0.5f }, { -0.5f, -0.5f,  0.5f }},
		{{ -0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }},
		{{  0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }},
	};

	// Face normals (right, left, top, bottom, front, back)
	const glm::vec3 faceNormals[6] = {
		{  1.0f,  0.0f,  0.0f },
		{ -1.0f,  0.0f,  0.0f },
		{  0.0f,  1.0f,  0.0f },
		{  0.0f, -1.0f,  0.0f },
		{  0.0f,  0.0f,  1.0f },
		{  0.0f,  0.0f, -1.0f },
	};

	// Face directions (right, left, top, bottom, front, back)
	const glm::ivec3 faceDirections[6] = {
		{ 1,  0,  0},
		{-1,  0,  0},
		{ 0,  1,  0},
		{ 0, -1,  0},
		{ 0,  0,  1},
		{ 0,  0, -1},
	};

	std::unique_ptr<Mesh> mesh;
	bool rebuildMesh = true;

	int getVoxelIndex(const glm::ivec3& chunkPosition) const;
	bool isValidPosition(const glm::ivec3& chunkPosition) const;

	void buildMesh(const ChunkNeighbors& neighbors);
};