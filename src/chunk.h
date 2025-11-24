#pragma once

#include "shader.h"
#include "structs.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_set>
#include <memory>

class Mesh;

static const int CHUNK_SIZE = 32;

class Chunk {
public:
	Chunk();
	~Chunk();

	void draw(const glm::ivec3 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material, const Light& light);

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