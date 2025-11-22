#include "world.h"
#include "shader.h"
#include "primitives/mesh.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_set>
#include <memory>
#include <stdexcept>
#include <iostream>

World::World() : mesh(nullptr) {
	// Flat 32x32x32 world
	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 32; y++)
		{
			for (int z = 0; z < 64; z++)
			{
				voxels.insert(glm::ivec3(x, y, z));
			}
		}
	}

	buildMesh();
}

World::~World() {

}

void World::draw(const glm::mat4& view, const glm::mat4& projection, Shader& shader) {
	if (!mesh) {
		buildMesh();
	}

	mesh->draw(glm::vec3(0.0f, 0.0f, 0.0f), view, projection, shader);
}

bool World::hasVoxel(const glm::ivec3& position) {
	return voxels.find(position) != voxels.end();
}

void World::addVoxel(const glm::ivec3& position) {
	voxels.insert(position);
	mesh = nullptr;
}

void World::removeVoxel(const glm::ivec3& position) {
	voxels.erase(position);
	mesh = nullptr;
}

void World::clearVoxels() {
	voxels.clear();
	mesh = nullptr;
}

// Add face culling later for optimization
void World::buildMesh() {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	// Define the vertices (position)
	const glm::vec3 voxelPositions[8] = {
		{ -0.5f, -0.5f, -0.5f },
		{  0.5f, -0.5f, -0.5f },
		{  0.5f,  0.5f, -0.5f },
		{ -0.5f,  0.5f, -0.5f },
		{ -0.5f, -0.5f,  0.5f },
		{  0.5f, -0.5f,  0.5f },
		{  0.5f,  0.5f,  0.5f },
		{ -0.5f,  0.5f,  0.5f }
	};

	// Define the vertices (color)
	const glm::vec3 voxelColors[8] = {
		{ 1.0f, 0.00f, 0.0f },
		{ 1.0f, 0.65f, 0.0f },
		{ 1.0f, 1.00f, 0.0f },
		{ 0.0f, 1.00f, 0.0f },
		{ 0.0f, 0.00f, 1.0f },
		{ 0.5f, 0.00f, 1.0f },
		{ 0.4f, 0.30f, 0.0f },
		{ 0.0f, 1.00f, 0.7f }
	};

	// Define the indices (12 triangles, arranged by face)
	const unsigned int voxelIndices[12][3] = {
		{0, 1, 2}, {2, 3, 0},
		{4, 5, 6}, {6, 7, 4},
		{4, 5, 1}, {1, 0, 4},
		{6, 7, 3}, {3, 2, 6},
		{4, 7, 3}, {3, 0, 4},
		{1, 5, 6}, {6, 2, 1}
	};

	for (const auto& voxelPos : voxels) {
		unsigned int baseIndex = vertices.size();

		// Add vertices
		for (int i = 0; i < 8; i++) {
			Vertex vertex{
				voxelPositions[i] + glm::vec3(voxelPos),
				voxelColors[i]
			};

			vertices.push_back(vertex);
		}

		// Add indices
		for (int i = 0; i < 12; i++) {
			indices.push_back(baseIndex + voxelIndices[i][0]);
			indices.push_back(baseIndex + voxelIndices[i][1]);
			indices.push_back(baseIndex + voxelIndices[i][2]);
		}
	}

	std::cout << "Built mesh with " << vertices.size() << " vertices and " << indices.size() / 3 << " triangles." << std::endl;

	mesh = std::make_unique<Mesh>(vertices, indices);
}
