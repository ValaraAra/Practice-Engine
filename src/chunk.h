#pragma once

#include "shader.h"
#include "structs.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_set>
#include <memory>

class Mesh;

static const int CHUNK_SIZE = 32;
static const int MAX_HEIGHT = 128;

class Chunk {
public:
	Chunk();
	~Chunk();

	void draw(const glm::ivec2 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material);

	bool isGenerated() const {
		return generated;
	}
	bool setGenerated() {
		return generated = true;
	}

	bool hasVoxel(const glm::ivec3& position);
	void addVoxel(const glm::ivec3& chunkPosition, const glm::vec3& color = glm::vec3(1.0f));
	void setVoxelColor(const glm::ivec3& chunkPosition, const glm::vec3& color);
	void removeVoxel(const glm::ivec3& position);
	void clearVoxels();

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

	int getVoxelIndex(const glm::ivec3& chunkPosition);
	bool isValidPosition(const glm::ivec3& chunkPosition);

	void buildMesh();
};