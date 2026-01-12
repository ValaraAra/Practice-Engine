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
#include <thread>
#include <tracy/Tracy.hpp>

Chunk::Chunk() : mesh(nullptr) {

}

Chunk::~Chunk() {
	if (meshThread && meshThread->joinable()) {
		meshThread->join();
	}
}

void Chunk::draw(const glm::ivec2 offset, const ChunkNeighbors& neighbors, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material) {
	ZoneScopedN("Chunk Draw");
	if (voxelCount == 0) {
		return;
	}

	if (meshNeedsUpdate.load() && !rebuildingMesh.load()) {
		ZoneScopedN("Mesh Rebuild Start");
		rebuildingMesh.store(true);

		// Wait for previous mesh thread to finish
		if (meshThread && meshThread->joinable()) {
			meshThread->join();
		}

		// Start new mesh thread
		meshThread.emplace([this, neighbors]() {
			tracy::SetThreadName("Mesh Rebuild Thread");
			ZoneScopedN("Mesh Rebuild");
			try {
				// Initial mesh update
				updateMesh(neighbors);

				// Process pending operations and update mesh again (if needed)
				if (!pendingOperations.empty()) {
					{
						std::lock_guard<std::mutex> lock(pendingOperationsMutex);
						while (!pendingOperations.empty()) {
							pendingOperations.front()();
							pendingOperations.pop();
						}
					}

					updateMesh(neighbors);
				}

				// Mark as done
				meshNeedsUpdate.store(false);
				rebuildingMesh.store(false);
			}
			catch (const std::exception& error) {
				std::cerr << "Error during chunk mesh update: " << error.what() << std::endl;
				rebuildingMesh.store(false);
			}
		});
	}

	if (meshDataReady.load()) {
		ZoneScopedN("Mesh Upload");
		std::lock_guard<std::mutex> lock(meshMutex);
		mesh = std::make_unique<Mesh>(pendingVertices, pendingIndices);

		meshDataReady.store(false);
	}

	if (mesh == nullptr) {
		return;
	}

	{
		ZoneScopedN("Mesh Draw");
		std::lock_guard<std::mutex> lock(meshMutex);
		mesh->draw(glm::vec3(offset.x, 0.0f, offset.y), view, projection, shader, material);
	}
}

// Utility functions
static bool isBorderVoxel(glm::ivec3 position) {
	return position.x == 0 || position.x == CHUNK_SIZE - 1 || position.z == 0 || position.z == CHUNK_SIZE - 1;
}

static bool isAdjacentBorderVoxel(glm::ivec3 position) {
	return position.x == -1 || position.x == CHUNK_SIZE || position.z == -1 || position.z == CHUNK_SIZE;
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

// Voxel manipulation
void Chunk::setVoxelType(const glm::ivec3& chunkPosition, const VoxelType type) {
	if (!isValidPosition(chunkPosition)) {
		return;
	}

	if (rebuildingMesh.load()) {
		std::lock_guard<std::mutex> lock(pendingOperationsMutex);

		pendingOperations.push([this, chunkPosition, type]() {
			performSetVoxelType(chunkPosition, type);
		});

		return;
	}

	performSetVoxelType(chunkPosition, type);
}

void Chunk::performSetVoxelType(const glm::ivec3& chunkPosition, const VoxelType type) {
	Voxel& voxel = voxels[getVoxelIndex(chunkPosition)];

	if (voxel.type == type) {
		return;
	}

	if (type == VoxelType::EMPTY) {
		std::lock_guard<std::mutex> lock(voxelFaceMutex);

		voxel.flags = 0;
		voxelCount--;
	}
	else if (voxel.type == VoxelType::EMPTY) {
		voxelCount++;
	}

	voxel.type = type;
	meshNeedsUpdate.store(true);
}

void Chunk::clearVoxels() {
	if (rebuildingMesh.load()) {
		std::lock_guard<std::mutex> lock(pendingOperationsMutex);

		pendingOperations.push([this]() {
			performClearVoxels();
		});

		return;
	}

	performClearVoxels();
}

void Chunk::performClearVoxels() {
	std::lock_guard<std::mutex> lock(voxelFaceMutex);

	for (int i = 0; i < maxVoxels; i++) {
		Voxel& voxel = voxels[i];
		voxel.flags = 0;
		voxel.type = VoxelType::EMPTY;
	}

	voxelCount = 0;
	meshNeedsUpdate.store(true);
}


void Chunk::calculateFaceVisibility(const ChunkNeighbors& neighbors) {
	ZoneScopedN("Face Visibility");
	std::lock_guard<std::mutex> lock(voxelFaceMutex);

	// Reset all face visibility flags
	for (int i = 0; i < maxVoxels; i++) {
		voxels[i].flags = 0;
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
					VoxelFlags::setFaceExposed(voxel.flags, VoxelFlags::FACE_FLAGS[face], true);
				}
			}
		}
	}
}

// Could be optimized further
void Chunk::buildMesh() {
	ZoneScopedN("Build Mesh");
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
						vertices.emplace_back(
							faceVertices[face][i] + voxelPosFull,
							voxelCol,
							faceNormals[face]
						);
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

	pendingVertices = std::move(vertices);
	pendingIndices = std::move(indices);
	meshDataReady.store(true);
}

void Chunk::updateMesh(const ChunkNeighbors& neighbors) {
	ZoneScopedN("Update Mesh");
	calculateFaceVisibility(neighbors);
	buildMesh();
}

void Chunk::updateMeshBorder(const Chunk* neighbor, const glm::ivec2& direction) {
	if (!neighbor) {
		return;
	}

	ZoneScopedN("Update Mesh Border");

	std::lock_guard<std::mutex> lock(voxelFaceMutex);

	if (direction == glm::ivec2(-1, 0)) {
		int x = 0;
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

				// Update left face visibility
				glm::ivec3 adjacentPos = glm::ivec3(CHUNK_SIZE - 1, y, z);
				VoxelFlags::setFaceExposed(voxel.flags, VoxelFlags::LEFT_EXPOSED, !neighbor->hasVoxel(adjacentPos));
			}
		}
	}
	else if (direction == glm::ivec2(1, 0)) {
		int x = CHUNK_SIZE - 1;
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

				// Update right face visibility
				glm::ivec3 adjacentPos = glm::ivec3(0, y, z);
				VoxelFlags::setFaceExposed(voxel.flags, VoxelFlags::RIGHT_EXPOSED, !neighbor->hasVoxel(adjacentPos));
			}
		}
	}
	else if (direction == glm::ivec2(0, -1)) {
		int z = 0;
		for (int y = 0; y < MAX_HEIGHT; y++)
		{
			for (int x = 0; x < CHUNK_SIZE; x++)
			{
				glm::ivec3 voxelPos = glm::ivec3(x, y, z);
				Voxel& voxel = voxels[getVoxelIndex(voxelPos)];

				// Skip empty voxels
				if (voxel.type == VoxelType::EMPTY) {
					continue;
				}

				// Update right face visibility
				glm::ivec3 adjacentPos = glm::ivec3(x, y, CHUNK_SIZE - 1);
				VoxelFlags::setFaceExposed(voxel.flags, VoxelFlags::BACK_EXPOSED, !neighbor->hasVoxel(adjacentPos));
			}
		}
	}
	else if (direction == glm::ivec2(0, 1)) {
		int z = CHUNK_SIZE - 1;
		for (int y = 0; y < MAX_HEIGHT; y++)
		{
			for (int x = 0; x < CHUNK_SIZE; x++)
			{
				glm::ivec3 voxelPos = glm::ivec3(x, y, z);
				Voxel& voxel = voxels[getVoxelIndex(voxelPos)];

				// Skip empty voxels
				if (voxel.type == VoxelType::EMPTY) {
					continue;
				}

				// Update right face visibility
				glm::ivec3 adjacentPos = glm::ivec3(x, y, 0);
				VoxelFlags::setFaceExposed(voxel.flags, VoxelFlags::FRONT_EXPOSED, !neighbor->hasVoxel(adjacentPos));
			}
		}
	}

	buildMesh();
}
