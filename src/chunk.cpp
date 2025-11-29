#include "chunk.h"
#include "shader.h"
#include "primitives/mesh.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <array>
#include <chrono>

Chunk::Chunk() : mesh(nullptr) {

}

Chunk::~Chunk() {

}

void Chunk::draw(const glm::ivec2 offset, const ChunkNeighbors& neighbors, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material) {
	if (voxelCount == 0) {
		return;
	}

	if (rebuildMesh) {
		buildMesh(neighbors);
	}

	if (!mesh) {
		throw std::runtime_error("Chunk mesh is null!");
	}

	mesh->draw(glm::vec3(offset.x, 0.0f, offset.y), view, projection, shader, material);
}

bool Chunk::isValidPosition(const glm::ivec3& chunkPosition) const {
	if (chunkPosition.x < 0 || chunkPosition.x >= CHUNK_SIZE || chunkPosition.y < 0 || chunkPosition.y >= MAX_HEIGHT || chunkPosition.z < 0 || chunkPosition.z >= CHUNK_SIZE) {
		return false;
	}

	return true;
}

int Chunk::getVoxelIndex(const glm::ivec3& chunkPosition) const {
	return chunkPosition.x + chunkPosition.y * CHUNK_SIZE + chunkPosition.z * CHUNK_SIZE * MAX_HEIGHT;
}

bool Chunk::hasVoxel(const glm::ivec3& chunkPosition) const {
	if (!isValidPosition(chunkPosition)) {
		return false;
	}
	
	return voxels[getVoxelIndex(chunkPosition)].type != VoxelType::EMPTY;
}

void Chunk::addVoxel(const glm::ivec3& chunkPosition, const VoxelType type) {
	if (!isValidPosition(chunkPosition)) {
		return;
	}

	Voxel& voxel = voxels[getVoxelIndex(chunkPosition)];

	voxel.flags = 0;
	voxel.type = type;

	voxelCount++;
	rebuildMesh = true;
}

void Chunk::setVoxelType(const glm::ivec3& chunkPosition, const VoxelType type) {
	if (!isValidPosition(chunkPosition)) {
		return;
	}

	Voxel& voxel = voxels[getVoxelIndex(chunkPosition)];
	voxel.type = type;

	rebuildMesh = true;
}

void Chunk::removeVoxel(const glm::ivec3& chunkPosition) {
	if (!isValidPosition(chunkPosition)) {
		return;
	}

	Voxel& voxel = voxels[getVoxelIndex(chunkPosition)];

	if (voxel.type == VoxelType::EMPTY) {
		return;
	}

	voxel.flags = 0;
	voxel.type = VoxelType::EMPTY;

	voxelCount--;
	rebuildMesh = true;
}

void Chunk::clearVoxels() {
	for (int i = 0; i < maxVoxels; i++) {
		Voxel& voxel = voxels[i];
		voxel.flags = 0;
		voxel.type = VoxelType::EMPTY;
	}

	voxelCount = 0;
	rebuildMesh = true;
}

static bool isBorderVoxel(glm::ivec3 position) {
	return position.x == 0 || position.x == CHUNK_SIZE - 1 || position.z == 0 || position.z == CHUNK_SIZE - 1;
}

static bool isAdjacentBorderVoxel(glm::ivec3 position) {
	return position.x == -1 || position.x == CHUNK_SIZE || position.z == -1 || position.z == CHUNK_SIZE;
}

void Chunk::calculateFaceVisibility(const ChunkNeighbors& neighbors) {
	for (int i = 0; i < maxVoxels; i++) {
		Voxel& voxel = voxels[i];
		voxel.flags = 0;
	}

	for (int x = 0; x < CHUNK_SIZE; x++)
	{
		for (int y = 0; y < MAX_HEIGHT; y++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				glm::ivec3 voxelPos = glm::ivec3(x, y, z);
				Voxel& voxel = voxels[getVoxelIndex(voxelPos)];

				// Skip empty voxels
				if (voxel.type == 0) {
					continue;
				}

				glm::vec3 voxelCol = voxelTypeData[voxel.type].color;

				for (int face = 0; face < 6; face++) {
					// Check for an adjacent voxel
					glm::ivec3 adjacentPos = glm::ivec3(x + faceDirections[face].x, y + faceDirections[face].y, z + faceDirections[face].z);

					// Skip if adjacent voxel has invalid Y position
					if (adjacentPos.y < 0 || adjacentPos.y >= MAX_HEIGHT) {
						continue;
					}

					// Current chunk checks
					if (adjacentPos.x >= 0 && adjacentPos.x < CHUNK_SIZE && adjacentPos.z >= 0 && adjacentPos.z < CHUNK_SIZE)
					{
						Voxel& adjacentVoxel = voxels[getVoxelIndex(adjacentPos)];

						if (adjacentVoxel.type != VoxelType::EMPTY) {
							continue;
						}
					}

					// Neighbor chunk checks
					else if (isAdjacentBorderVoxel(adjacentPos)) {
						Chunk* neighborChunk = nullptr;

						if (adjacentPos.x == -1) {
							neighborChunk = neighbors.nx;
							adjacentPos.x += CHUNK_SIZE;
						}
						else if (adjacentPos.x == CHUNK_SIZE) {
							neighborChunk = neighbors.px;
							adjacentPos.x -= CHUNK_SIZE;
						}
						else if (adjacentPos.z == -1) {
							neighborChunk = neighbors.ny;
							adjacentPos.z += CHUNK_SIZE;
						}
						else if (adjacentPos.z == CHUNK_SIZE) {
							neighborChunk = neighbors.py;
							adjacentPos.z -= CHUNK_SIZE;
						}

						if (neighborChunk && neighborChunk->hasVoxel(adjacentPos)) {
							continue;
						}
					}

					// Mark face as visible
					//std::cout << "Voxel at (" << x << ", " << y << ", " << z << ") has face " << face << " exposed." << std::endl;
					VoxelFlags::setFaceExposed(voxel.flags, VoxelFlags::FACE_FLAGS[face], true);
				}
			}
		}
	}
}

// Could be optimized further
void Chunk::buildMesh(const ChunkNeighbors& neighbors) {
	auto faceVisStartTime = std::chrono::high_resolution_clock::now();
	calculateFaceVisibility(neighbors);
	auto faceVisEndTime = std::chrono::high_resolution_clock::now();

	auto buildStartTime = std::chrono::high_resolution_clock::now();

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
				Voxel& voxel = voxels[getVoxelIndex(voxelPos)];

				// Skip empty voxels
				if (voxel.type == VoxelType::EMPTY) {
					continue;
				}


				glm::vec3 voxelPosFull = glm::vec3(voxelPos);
				glm::vec3 voxelCol = voxelTypeData[voxel.type].color;

				for (int face = 0; face < 6; face++) {
					if (!VoxelFlags::isFaceExposed(voxel.flags, VoxelFlags::FACE_FLAGS[face])) {
						continue;
					}

					unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

					// Add face vertices (4 vertices, quad)
					for (int i = 0; i < 4; i++) {
						Vertex vertex{
							faceVertices[face][i] + voxelPosFull,
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

	//std::cout << "Built mesh with " << vertices.size() << " vertices and " << indices.size() / 3 << " triangles." << std::endl;

	mesh = std::make_unique<Mesh>(vertices, indices);
	rebuildMesh = false;

	auto buildEndTime = std::chrono::high_resolution_clock::now();

	auto faceVisDuration = std::chrono::duration_cast<std::chrono::microseconds>(faceVisEndTime - faceVisStartTime);
	auto buildDuration = std::chrono::duration_cast<std::chrono::microseconds>(buildEndTime - buildStartTime);

	std::cout << "Chunk buildMesh: Face visibility calculation took " << faceVisDuration.count() << " us, mesh building took " << buildDuration.count() << " us." << std::endl;
}

// TODO
void Chunk::updateMesh(const Chunk* neighbor, const glm::ivec2& direction) {
	rebuildMesh = true;
}

// TODO
void Chunk::updateMeshBorder(const Chunk* neighbor, const glm::ivec2& direction) {
	rebuildMesh = true;
}
