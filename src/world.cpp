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

// Could be optimized further
void World::buildMesh() {
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
	
	for (const auto& voxelPos : voxels) {
		for (int face = 0; face < 6; face++) {
			// Check for an adjacent voxel
			if (hasVoxel(voxelPos + faceDirections[face])) {
				continue;
			}
			
			unsigned int baseIndex = vertices.size();

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

	std::cout << "Built mesh with " << vertices.size() << " vertices and " << indices.size() / 3 << " triangles." << std::endl;

	mesh = std::make_unique<Mesh>(vertices, indices);
}
