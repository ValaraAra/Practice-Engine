#include "chunk.h"
#include "shader.h"
#include "primitives/mesh.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <memory>
#include <stdexcept>
#include <iostream>

Chunk::Chunk() : mesh(nullptr) {
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		voxels[i] = false;
	}

	buildMesh();
}

Chunk::~Chunk() {

}

void Chunk::draw(const glm::ivec3 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader) {
	if (!mesh) {
		buildMesh();
	}

	mesh->draw(glm::vec3(offset), view, projection, shader);
}

bool Chunk::isValidPosition(const glm::ivec3& chunkPosition) {
	if (chunkPosition.x < 0 || chunkPosition.x >= CHUNK_SIZE || chunkPosition.y < 0 || chunkPosition.y >= CHUNK_SIZE || chunkPosition.z < 0 || chunkPosition.z >= CHUNK_SIZE) {
		return false;
	}

	return true;
}

int Chunk::getVoxelIndex(const glm::ivec3& chunkPosition) {
	return chunkPosition.x + chunkPosition.y * CHUNK_SIZE + chunkPosition.z * CHUNK_SIZE * CHUNK_SIZE;
}

bool Chunk::hasVoxel(const glm::ivec3& chunkPosition) {
	if (!isValidPosition(chunkPosition)) {
		return false;
	}

	return voxels[getVoxelIndex(chunkPosition)];
}

void Chunk::addVoxel(const glm::ivec3& chunkPosition) {
	if (!isValidPosition(chunkPosition)) {
		return;
	}

	voxels[getVoxelIndex(chunkPosition)] = true;
	mesh = nullptr;
}

void Chunk::removeVoxel(const glm::ivec3& chunkPosition) {
	if (!isValidPosition(chunkPosition)) {
		return;
	}

	voxels[getVoxelIndex(chunkPosition)] = false;
	mesh = nullptr;
}

void Chunk::clearVoxels() {
	for (int i = 0; i < CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; i++) {
		voxels[i] = false;
	}

	mesh = nullptr;
}

// Could be optimized further
void Chunk::buildMesh() {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	// Face vertices (right, left, top, bottom, front, back)
	const glm::vec3 faceVertices[6][4] = {
		{{  0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f,  0.5f }},
		{{ -0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f, -0.5f }},
		{{ -0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }},
		{{ -0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f,  0.5f }, { -0.5f, -0.5f,  0.5f }},
		{{ -0.5f, -0.5f,  0.5f }, {  0.5f, -0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }},
		{{  0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }},
	};

	// Face colors (right, left, top, bottom, front, back)
	const glm::vec3 faceColors[6][4] = {
		{{ 0.5f, 0.00f, 1.0f }, { 1.0f, 0.65f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.4f, 0.3f, 0.0f }},
		{{ 1.0f, 0.00f, 0.0f }, { 0.0f, 0.00f, 1.0f }, { 0.0f, 1.0f, 0.7f }, { 0.0f, 1.0f, 0.0f }},
		{{ 0.0f, 1.00f, 0.7f }, { 0.4f, 0.30f, 0.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
		{{ 1.0f, 0.00f, 0.0f }, { 1.0f, 0.65f, 0.0f }, { 0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
		{{ 0.0f, 0.00f, 1.0f }, { 0.5f, 0.00f, 1.0f }, { 0.4f, 0.3f, 0.0f }, { 0.0f, 1.0f, 0.7f }},
		{{ 1.0f, 0.65f, 0.0f }, { 1.0f, 0.00f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }},
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

	for (int x = 0; x < CHUNK_SIZE; x++)
	{
		for (int y = 0; y < CHUNK_SIZE; y++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				glm::ivec3 voxelPos = glm::ivec3(x, y, z);

				// Skip empty voxels
				if (!hasVoxel(voxelPos)) {
					continue;
				}

				for (int face = 0; face < 6; face++) {
					// Check for an adjacent voxel
					if (hasVoxel(voxelPos + faceDirections[face])) {
						continue;
					}

					unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

					// Add face vertices (4 vertices, quad)
					for (int i = 0; i < 4; i++) {
						Vertex vertex{
							faceVertices[face][i] + glm::vec3(voxelPos),
							faceColors[face][i]
						};
						vertices.push_back(vertex);
					}

					// Add face indices (2 triangles, quad)
					indices.push_back(baseIndex + 0);
					indices.push_back(baseIndex + 1);
					indices.push_back(baseIndex + 2);

					indices.push_back(baseIndex + 2);
					indices.push_back(baseIndex + 3);
					indices.push_back(baseIndex + 0);
				}
			}
		}
	}

	std::cout << "Built mesh with " << vertices.size() << " vertices and " << indices.size() / 3 << " triangles." << std::endl;

	mesh = std::make_unique<Mesh>(vertices, indices);
}
