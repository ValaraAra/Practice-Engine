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
#include <ranges>

Chunk::Chunk() : mesh(nullptr) {

}

Chunk::~Chunk() {
	if (meshThread && meshThread->joinable()) {
		meshThread->join();
	}
}

void Chunk::update(const ChunkNeighbors& neighbors) {
	ZoneScopedN("Chunk Update");

	if (meshState.load() == MeshState::NONE) {
		if (voxelCount > 0) {
			dirty.store(true);
		}
		else {
			return;
		}
	}

	if (dirty.load()) {
		ZoneScopedN("Mesh Rebuild Start");
		dirty.store(false);

		// Wait for previous mesh thread to finish
		if (meshThread && meshThread->joinable()) {
			meshThread->join();
		}

		// Check for and handle empty chunk
		if (voxelCount <= 0) {
			std::lock_guard<std::mutex> lock(meshMutex);

			meshState.store(MeshState::NONE);
			mesh = nullptr;

			return;
		}

		// Start new mesh thread
		meshState.store(MeshState::BUILDING);

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

				// Mark as ready for upload
				meshState.store(MeshState::HANDOFF);
			}
			catch (const std::exception& error) {
				std::cerr << "Error during chunk mesh update: " << error.what() << std::endl;
				dirty.store(true);
			}
		});
	}

	// Upload mesh if ready
	if (meshState.load() == MeshState::HANDOFF) {
		ZoneScopedN("Mesh Upload");
		meshState.store(MeshState::UPLOADING);

		std::lock_guard<std::mutex> lock(meshMutex);
		mesh = std::make_unique<Mesh>(std::move(pendingFaces));

		meshState.store(MeshState::READY);
	}
}

void Chunk::draw(const glm::ivec2 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material) {
	ZoneScopedN("Chunk Draw");
	std::lock_guard<std::mutex> lock(meshMutex);

	if (mesh) {
		mesh->draw(glm::vec3(offset.x, 0.0f, offset.y), view, projection, shader, material);
	}
}

// Utility functions
bool Chunk::isBorderVoxel(const glm::ivec3& position) {
	return position.x == 0 || position.x == CHUNK_SIZE - 1 || position.z == 0 || position.z == CHUNK_SIZE - 1;
}

bool Chunk::isAdjacentBorderVoxel(const glm::ivec3& position) {
	return position.x == -1 || position.x == CHUNK_SIZE || position.z == -1 || position.z == CHUNK_SIZE;
}

bool Chunk::isValidPosition(const glm::ivec3& chunkPosition) {
	if (chunkPosition.x < 0 || chunkPosition.x >= CHUNK_SIZE || chunkPosition.y < 0 || chunkPosition.y >= MAX_HEIGHT || chunkPosition.z < 0 || chunkPosition.z >= CHUNK_SIZE) {
		return false;
	}

	return true;
}

glm::ivec3 Chunk::getChunkPosition(const int chunkIndex) const {
	return { chunkIndex % CHUNK_SIZE, (chunkIndex / CHUNK_SIZE) % MAX_HEIGHT, chunkIndex / (CHUNK_SIZE * MAX_HEIGHT) };
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

	if (meshState.load() == MeshState::BUILDING) {
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
	dirty.store(true);
}

void Chunk::clearVoxels() {
	if (meshState.load() == MeshState::BUILDING) {
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
	dirty.store(true);
}

// This needs serious improvements
void Chunk::calculateFaceVisibility(const ChunkNeighbors& neighbors) {
	ZoneScopedN("Face Visibility");
	std::lock_guard<std::mutex> lock(voxelFaceMutex);

	// Reset all face visibility flags
	for (int i = 0; i < maxVoxels; i++) {
		voxels[i].flags = 0;
	}

	int pxBorders = 0;
	int nxBorders = 0;
	int pzBorders = 0;
	int nzBorders = 0;

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

				for (int face = 0; face < 6; face++) {
					// Check for an adjacent voxel
					glm::ivec3 adjacentPos = glm::ivec3(x + DirectionVectors::arr[face].x, y + DirectionVectors::arr[face].y, z + DirectionVectors::arr[face].z);

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
						std::shared_ptr<Chunk> neighborChunk;

						if (adjacentPos.x == CHUNK_SIZE) {
							neighborChunk = neighbors.px;
							adjacentPos.x -= CHUNK_SIZE;
							pxBorders++;
						}
						else if (adjacentPos.x == -1) {
							neighborChunk = neighbors.nx;
							adjacentPos.x += CHUNK_SIZE;
							nxBorders++;
						}
						else if (adjacentPos.z == CHUNK_SIZE) {
							neighborChunk = neighbors.pz;
							adjacentPos.z -= CHUNK_SIZE;
							pzBorders++;
						}
						else if (adjacentPos.z == -1) {
							neighborChunk = neighbors.nz;
							adjacentPos.z += CHUNK_SIZE;
							nzBorders++;
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

	// Log border voxel counts for debugging
	ZoneValue(pxBorders);
	ZoneValue(nxBorders);
	ZoneValue(pzBorders);
	ZoneValue(nzBorders);
}

// Could be optimized further
void Chunk::buildMesh() {
	ZoneScopedN("Build Mesh");

	std::vector<Face> faces;

	for (const auto& [index, voxel] : std::views::enumerate(voxels)) {
		if (voxel.type == VoxelType::EMPTY) {
			continue;
		}

		glm::ivec3 voxelPos = getChunkPosition(index);

		// Iterate faces and add exposed
		for (int i = 0; i < 6; i++) {
			if (!VoxelFlags::isFaceExposed(voxel.flags, VoxelFlags::FACE_FLAGS[i])) {
				continue;
			}

			Face face;
			FacePacked::setPosition(face, voxelPos);
			FacePacked::setFace(face, static_cast<uint8_t>(i));
			FacePacked::setTexID(face, static_cast<uint8_t>(voxel.type));
			faces.emplace_back(face);
		}
	}

	pendingFaces = std::move(faces);
}

void Chunk::updateMesh(const ChunkNeighbors& neighbors) {
	ZoneScopedN("Update Mesh");

	calculateFaceVisibility(neighbors);
	buildMesh();
}

void Chunk::updateMeshBorder(const std::shared_ptr<Chunk> neighbor, const Direction2D direction) {
	if (!neighbor) {
		return;
	}

	ZoneScopedN("Update Mesh Border");

	std::lock_guard<std::mutex> lock(voxelFaceMutex);

	if (direction == Direction2D::PX) {
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
	else if (direction == Direction2D::NX) {
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
	else if (direction == Direction2D::PZ) {
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

				// Update front face visibility
				glm::ivec3 adjacentPos = glm::ivec3(x, y, 0);
				VoxelFlags::setFaceExposed(voxel.flags, VoxelFlags::FRONT_EXPOSED, !neighbor->hasVoxel(adjacentPos));
			}
		}
	}
	else if (direction == Direction2D::NZ) {
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

				// Update back face visibility
				glm::ivec3 adjacentPos = glm::ivec3(x, y, CHUNK_SIZE - 1);
				VoxelFlags::setFaceExposed(voxel.flags, VoxelFlags::BACK_EXPOSED, !neighbor->hasVoxel(adjacentPos));
			}
		}
	}

	buildMesh();
}

// Needs improvement. Should be doing face vis updates in a better way
void Chunk::updateMeshBorderNew(const std::shared_ptr<Chunk> neighbor, const Direction2D direction) {
	if (!neighbor) {
		return;
	}

	ZoneScopedN("Update Mesh Border");

	std::lock_guard<std::mutex> lock(voxelFaceMutex);

	const BorderInfo borderInfo = borderInfoTable[static_cast<size_t>(direction)];

	glm::ivec3 voxelPos(0);
	glm::ivec3 neighborPos(0);

	voxelPos[static_cast<int>(borderInfo.fixedAxis)] = borderInfo.fixedValue;
	neighborPos[static_cast<int>(borderInfo.fixedAxis)] = borderInfo.neighborValue;

	for (int y = 0; y < MAX_HEIGHT; y++)
	{
		for (int sweep = 0; sweep < CHUNK_SIZE; sweep++)
		{
			// Update positions
			voxelPos[static_cast<int>(borderInfo.updateAxis)] = sweep;
			voxelPos.y = y;

			neighborPos[static_cast<int>(borderInfo.updateAxis)] = sweep;
			neighborPos.y = y;

			// Get the current voxel
			Voxel& voxel = voxels[getVoxelIndex(voxelPos)];

			// Skip empty voxels
			if (voxel.type == VoxelType::EMPTY) {
				continue;
			}

			// Update face visibility
			VoxelFlags::setFaceExposed(voxel.flags, borderInfo.faceFlag, !neighbor->hasVoxel(neighborPos));
		}
	}

	buildMesh();
}
