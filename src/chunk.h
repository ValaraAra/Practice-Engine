#pragma once

#include "shader.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_set>
#include <memory>

class Mesh;

// Temporary hasher to use unordered_set until switch to 3d arrays
struct ivec3Hasher {
	size_t operator()(const glm::ivec3& vec) const noexcept {
		size_t hashX = std::hash<int>{}(vec.x);
		size_t hashY = std::hash<int>{}(vec.y);
		size_t hashZ = std::hash<int>{}(vec.z);

		return hashX ^ (hashY << 1) ^ (hashZ << 2);
	}
};

struct Voxel {
	bool present = false;
	glm::vec3 color = glm::vec3(1.0f);
};

static const int CHUNK_SIZE = 32;

class Chunk {
public:
	Chunk();
	~Chunk();

	void draw(const glm::ivec3 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader);

	bool hasVoxel(const glm::ivec3& position);
	void addVoxel(const glm::ivec3& chunkPosition, const glm::vec3& color = glm::vec3(1.0f));
	void setVoxelColor(const glm::ivec3& chunkPosition, const glm::vec3& color);
	void removeVoxel(const glm::ivec3& position);
	void clearVoxels();

private:
	Voxel voxels[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE];
	int voxelCount = 0;

	std::unique_ptr<Mesh> mesh;

	int getVoxelIndex(const glm::ivec3& chunkPosition);
	bool isValidPosition(const glm::ivec3& chunkPosition);

	void buildMesh();
};