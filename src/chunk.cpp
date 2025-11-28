#include "chunk.h"
#include "shader.h"
#include "primitives/mesh.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <array>

Chunk::Chunk() : mesh(nullptr) {

}

Chunk::~Chunk() {

}

void Chunk::draw(const glm::ivec2 offset, const ChunkNeighbors& neighbors, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material) {
	if (voxelCount == 0) {
		return;
	}

	if (!rebuildMesh) {
		buildMesh(neighbors);
	}

	mesh->draw(glm::vec3(offset.x, 0.0f, offset.y), view, projection, shader, material);
}

bool Chunk::isValidPosition(const glm::ivec3& chunkPosition) {
	if (chunkPosition.x < 0 || chunkPosition.x >= CHUNK_SIZE || chunkPosition.y < 0 || chunkPosition.y >= MAX_HEIGHT || chunkPosition.z < 0 || chunkPosition.z >= CHUNK_SIZE) {
		return false;
	}

	return true;
}

int Chunk::getVoxelIndex(const glm::ivec3& chunkPosition) {
	return chunkPosition.x + chunkPosition.y * CHUNK_SIZE + chunkPosition.z * CHUNK_SIZE * MAX_HEIGHT;
}

bool Chunk::hasVoxel(const glm::ivec3& chunkPosition) {
	if (!isValidPosition(chunkPosition)) {
		return false;
	}

	return voxels[getVoxelIndex(chunkPosition)].present;
}

void Chunk::addVoxel(const glm::ivec3& chunkPosition, const glm::vec3& color) {
	if (!isValidPosition(chunkPosition)) {
		return;
	}

	voxels[getVoxelIndex(chunkPosition)] = { true, color };
	voxelCount++;
	rebuildMesh = true;
}

void Chunk::setVoxelColor(const glm::ivec3& chunkPosition, const glm::vec3& color) {
	if (!isValidPosition(chunkPosition)) {
		return;
	}

	voxels[getVoxelIndex(chunkPosition)].color = color;
	rebuildMesh = true;
}

void Chunk::removeVoxel(const glm::ivec3& chunkPosition) {
	if (!isValidPosition(chunkPosition)) {
		return;
	}

	voxels[getVoxelIndex(chunkPosition)].present = false;
	voxelCount--;
	rebuildMesh = true;
}

void Chunk::clearVoxels() {
	for (int i = 0; i < maxVoxels; i++) {
		voxels[i] = {};
	}

	voxelCount = 0;
	rebuildMesh = true;
}

// Could be optimized further
void Chunk::buildMesh(const ChunkNeighbors& neighbors) {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	//vertices.reserve(maxVoxels * 3);
	//indices.reserve(maxVoxels * 6);

	for (int x = 0; x < CHUNK_SIZE; x++)
	{
		for (int y = 0; y < MAX_HEIGHT; y++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				glm::ivec3 voxelPos = glm::ivec3(x, y, z);
				int voxelIndex = getVoxelIndex(voxelPos);

				// Skip empty voxels
				if (!voxels[voxelIndex].present) {
					continue;
				}

				glm::vec3 voxelCol = voxels[voxelIndex].color;

				for (int face = 0; face < 6; face++) {
					// Check for an adjacent voxel
					int adjacentX = x + faceDirections[face].x;
					int adjacentY = y + faceDirections[face].y;
					int adjacentZ = z + faceDirections[face].z;

					// Skip if adjacent voxel is below the world
					if (adjacentY < 0) {
						continue;
					}

					// Current chunk checks
					if (adjacentX >= 0 && adjacentX < CHUNK_SIZE && adjacentY >= 0 && adjacentY < MAX_HEIGHT && adjacentZ >= 0 && adjacentZ < CHUNK_SIZE)
					{
						if (voxels[adjacentX + adjacentY * CHUNK_SIZE + adjacentZ * CHUNK_SIZE * MAX_HEIGHT].present) {
							continue;
						}
					}

					// Neighbor chunk checks
					else if (adjacentY >= 0 && adjacentY < MAX_HEIGHT) {
						Chunk* neighborChunk = nullptr;

						if (adjacentX < 0) {
							neighborChunk = neighbors.nx;
							adjacentX += CHUNK_SIZE;
						}
						else if (adjacentX >= CHUNK_SIZE) {
							neighborChunk = neighbors.px;
							adjacentX -= CHUNK_SIZE;
						}
						else if (adjacentZ < 0) {
							neighborChunk = neighbors.ny;
							adjacentZ += CHUNK_SIZE;
						}
						else if (adjacentZ >= CHUNK_SIZE) {
							neighborChunk = neighbors.py;
							adjacentZ -= CHUNK_SIZE;
						}

						if (neighborChunk) {
							if (neighborChunk->hasVoxel(glm::ivec3(adjacentX, adjacentY, adjacentZ))) {
								continue;
							}
						}
					}

					unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

					// Add face vertices (4 vertices, quad)
					for (int i = 0; i < 4; i++) {
						Vertex vertex{
							faceVertices[face][i] + glm::vec3(voxelPos),
							voxelCol,
							faceNormals[face]
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

void Chunk::updateMesh(const Chunk* neighbor, const glm::ivec2& direction) {
	rebuildMesh = true;
}
