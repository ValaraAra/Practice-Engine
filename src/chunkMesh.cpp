#include "chunkMesh.h"
#include "shader.h"
#include <tracy/Tracy.hpp>
#include <stdexcept>
#include <iostream>
#include <array>
#include <chrono>
#include <thread>
#include <ranges>
#include <vector>

ChunkMesh::ChunkMesh() : mesh(nullptr) {

}

ChunkMesh::~ChunkMesh() {

}

void ChunkMesh::update(const ChunkNeighbors& neighbors) {
	// Check neighbors for border updates
	{
		std::unique_lock lock(neighborsMutex);

		if (neighbors.px && neighborVersions.px != neighbors.px->getVersion()) {
			neighborsUpdated.set(static_cast<size_t>(Direction2D::PX));
		}
		if (neighbors.nx && neighborVersions.nx != neighbors.nx->getVersion()) {
			neighborsUpdated.set(static_cast<size_t>(Direction2D::NX));
		}
		if (neighbors.pz && neighborVersions.pz != neighbors.pz->getVersion()) {
			neighborsUpdated.set(static_cast<size_t>(Direction2D::PZ));
		}
		if (neighbors.nz && neighborVersions.nz != neighbors.nz->getVersion()) {
			neighborsUpdated.set(static_cast<size_t>(Direction2D::NZ));
		}
	}

	// Upload mesh if ready
	if (meshState.load() == MeshState::HANDOFF) {
		ZoneScopedN("Mesh Upload");

		meshState.store(MeshState::UPLOADING);

		// Convert face map to vector
		std::lock_guard<std::mutex> lock(faceMutex);
		std::vector<Face> faceVector;
		faceVector.reserve(faces.size());

		for (const auto& [key, value] : faces) {
			faceVector.push_back(value);
		}

		// Create new mesh
		std::lock_guard<std::mutex> lock2(meshMutex);
		mesh = std::make_unique<Mesh>(std::move(faceVector));
		meshValid.store(true);

		meshState.store(MeshState::READY);
	}
}

void ChunkMesh::draw(const glm::ivec2 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader, const Material& material) {
	ZoneScopedN("Chunk Draw");
	std::lock_guard<std::mutex> lock(meshMutex);

	if (mesh) {
		mesh->draw(glm::vec3(offset.x, 0.0f, offset.y), view, projection, shader, material);
	}
}

void ChunkMesh::clear() {
	std::unique_lock lock(neighborsMutex);
	std::lock_guard<std::mutex> lock2(faceMutex);
	std::lock_guard<std::mutex> lock3(meshMutex);

	meshState.store(MeshState::NONE);
	meshValid.store(false);
	mesh = nullptr;

	faces.clear();

	neighborVersions = {};
	neighborsUpdated.reset();
}

void ChunkMesh::remesh(const std::shared_ptr<Chunk> chunk, const ChunkNeighbors& neighbors) {
	ZoneScopedN("Remesh Chunk");

	// Handle empty chunk
	if (chunk->getVoxelCount() <= 0) {
		clear();
		return;
	}

	meshState.store(MeshState::BUILDING);

	std::array<std::shared_ptr<std::array<Voxel, CHUNK_SIZE* MAX_HEIGHT>>, 4> neighborBorderVoxels{};

	// Record current neighbor versions (should be doing this when neighbor data is collected)
	{
		std::unique_lock lock(neighborsMutex);

		if (neighbors.px) {
			neighborBorderVoxels[0] = std::make_shared<std::array<Voxel, CHUNK_SIZE* MAX_HEIGHT>>(neighbors.px->getBorderVoxels(Direction2D::NX));
			neighborVersions.px = neighbors.px->getVersion();
		}
		if (neighbors.nx) {
			neighborBorderVoxels[1] = std::make_shared<std::array<Voxel, CHUNK_SIZE* MAX_HEIGHT>>(neighbors.nx->getBorderVoxels(Direction2D::PX));
			neighborVersions.nx = neighbors.nx->getVersion();
		}
		if (neighbors.pz) {
			neighborBorderVoxels[2] = std::make_shared<std::array<Voxel, CHUNK_SIZE* MAX_HEIGHT>>(neighbors.pz->getBorderVoxels(Direction2D::NZ));
			neighborVersions.pz = neighbors.pz->getVersion();
		}
		if (neighbors.nz) {
			neighborBorderVoxels[3] = std::make_shared<std::array<Voxel, CHUNK_SIZE* MAX_HEIGHT>>(neighbors.nz->getBorderVoxels(Direction2D::PZ));
			neighborVersions.nz = neighbors.nz->getVersion();
		}

		neighborsUpdated.reset();
	}

	// Snapshot data and rebuild faces
	rebuildFaces(chunk->getVoxels(), neighborBorderVoxels);
	meshState.store(MeshState::HANDOFF);
}

void ChunkMesh::remeshBorders(const std::shared_ptr<Chunk> chunk, const ChunkNeighbors& neighbors) {
	ZoneScopedN("Remesh Borders");

	// Handle empty chunk
	if (chunk->getVoxelCount() <= 0) {
		clear();
		return;
	}

	meshState.store(MeshState::BUILDING);
	
	// Actually remesh borders
	{
		// Might cause blocking on main thread
		std::unique_lock lock(neighborsMutex);

		// Record current neighbor versions (should be doing this when neighbor data is collected)
		if (neighbors.px) {
			neighborVersions.px = neighbors.px->getVersion();
		}
		if (neighbors.nx) {
			neighborVersions.nx = neighbors.nx->getVersion();
		}
		if (neighbors.pz) {
			neighborVersions.pz = neighbors.pz->getVersion();
		}
		if (neighbors.nz) {
			neighborVersions.nz = neighbors.nz->getVersion();
		}

		// Snapshot data and rebuild borders
		if (neighborsUpdated.test(static_cast<size_t>(Direction2D::PX)) && neighbors.px) {
			rebuildBorderFaces(chunk->getBorderVoxels(Direction2D::PX), neighbors.px->getBorderVoxels(Direction2D::NX), Direction2D::PX);
		}
		if (neighborsUpdated.test(static_cast<size_t>(Direction2D::NX)) && neighbors.nx) {
			rebuildBorderFaces(chunk->getBorderVoxels(Direction2D::NX), neighbors.nx->getBorderVoxels(Direction2D::PX), Direction2D::NX);
		}
		if (neighborsUpdated.test(static_cast<size_t>(Direction2D::PZ)) && neighbors.pz) {
			rebuildBorderFaces(chunk->getBorderVoxels(Direction2D::PZ), neighbors.pz->getBorderVoxels(Direction2D::NZ), Direction2D::PZ);
		}
		if (neighborsUpdated.test(static_cast<size_t>(Direction2D::NZ)) && neighbors.nz) {
			rebuildBorderFaces(chunk->getBorderVoxels(Direction2D::NZ), neighbors.nz->getBorderVoxels(Direction2D::PZ), Direction2D::NZ);
		}

		neighborsUpdated.reset();
	}


	meshState.store(MeshState::HANDOFF);
}

// Needs improvement
void ChunkMesh::rebuildFaces(const std::array<Voxel, MAX_VOXELS>&& chunkVoxels, const std::array<std::shared_ptr<std::array<Voxel, CHUNK_SIZE* MAX_HEIGHT>>, 4> neighborBorderVoxels) {
	ZoneScopedN("Rebuild Faces");

	std::map<glm::ivec4, Face, ivec4Comparator> workingFaces;

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
				VoxelType voxelType = chunkVoxels[getVoxelIndex(voxelPos)].type;

				// Skip empty voxels
				if (voxelType == VoxelType::EMPTY) {
					continue;
				}

				// Check adjacent voxels
				for (int i = 0; i < 6; i++) {
					glm::ivec3 adjacentPos = glm::ivec3(x + DirectionVectors::arr[i].x, y + DirectionVectors::arr[i].y, z + DirectionVectors::arr[i].z);

					// Skip if adjacent voxel has invalid Y position
					if (adjacentPos.y < 0 || adjacentPos.y >= MAX_HEIGHT) {
						continue;
					}

					// Neighbor chunk checks
					if (isAdjacentBorderVoxel(adjacentPos)) {
						int neighborChunkIndex;

						if (adjacentPos.x == CHUNK_SIZE) {
							neighborChunkIndex = 0;
							adjacentPos.x -= CHUNK_SIZE;
							pxBorders++;
						}
						else if (adjacentPos.x == -1) {
							neighborChunkIndex = 1;
							adjacentPos.x += CHUNK_SIZE;
							nxBorders++;
						}
						else if (adjacentPos.z == CHUNK_SIZE) {
							neighborChunkIndex = 2;
							adjacentPos.z -= CHUNK_SIZE;
							pzBorders++;
						}
						else {
							neighborChunkIndex = 3;
							adjacentPos.z += CHUNK_SIZE;
							nzBorders++;
						}

						// Skip if neighbor adjacent voxel is non-empty
						std::shared_ptr<std::array<Voxel, CHUNK_SIZE * MAX_HEIGHT>> neighborChunk = neighborBorderVoxels[neighborChunkIndex];
						int borderIndex = adjacentPos.y * CHUNK_SIZE + ((neighborChunkIndex < 2) ? adjacentPos.z : adjacentPos.x);
						if (neighborBorderVoxels[neighborChunkIndex] != nullptr && neighborChunk->at(borderIndex).type != VoxelType::EMPTY) {
							continue;
						}
					}
					// Skip if local adjacent voxel is non-empty
					else if (chunkVoxels[getVoxelIndex(adjacentPos)].type != VoxelType::EMPTY) {
						continue;
					}

					// Face is visible
					Face faceCurrent;
					FacePacked::setPosition(faceCurrent, voxelPos);
					FacePacked::setFace(faceCurrent, static_cast<uint8_t>(static_cast<uint8_t>(i)));
					FacePacked::setTexID(faceCurrent, static_cast<uint8_t>(voxelType));

					workingFaces.emplace(glm::ivec4{ voxelPos, i }, faceCurrent);
				}
			}
		}
	}

	// Replace faces
	{
		std::lock_guard<std::mutex> lock(faceMutex);
		faces = std::move(workingFaces);
	}

	// Log border voxel counts for debugging
	ZoneValue(pxBorders);
	ZoneValue(nxBorders);
	ZoneValue(pzBorders);
	ZoneValue(nzBorders);
}

// Could probably be improved
void ChunkMesh::rebuildBorderFaces(const std::array<Voxel, CHUNK_SIZE * MAX_HEIGHT>&& chunkBorderVoxels, const std::array<Voxel, CHUNK_SIZE * MAX_HEIGHT>&& neighborBorderVoxels, const Direction2D direction) {
	ZoneScopedN("Rebuild Border Faces");

	std::vector<glm::ivec4> removedFaces;
	std::vector<std::pair<glm::ivec4, Face>> addedFaces;

	uint8_t directionInt = static_cast<uint8_t>(direction);
	const BorderInfo borderInfo = BorderInfoTable[directionInt];

	glm::ivec3 voxelPos(0);
	glm::ivec3 neighborPos(0);

	voxelPos[static_cast<int>(borderInfo.fixedAxis)] = borderInfo.fixedValue;
	neighborPos[static_cast<int>(borderInfo.fixedAxis)] = borderInfo.neighborValue;

	for (int y = 0; y < MAX_HEIGHT; y++)
	{
		// Update y axis value
		voxelPos.y = y;
		neighborPos.y = y;

		int yOffset = y * CHUNK_SIZE;

		for (int sweep = 0; sweep < CHUNK_SIZE; sweep++)
		{
			// Update sweep axis value
			voxelPos[static_cast<int>(borderInfo.updateAxis)] = sweep;
			neighborPos[static_cast<int>(borderInfo.updateAxis)] = sweep;

			// Get voxel types
			VoxelType voxelType = chunkBorderVoxels[yOffset + sweep].type;
			VoxelType neighborVoxelType = neighborBorderVoxels[yOffset + sweep].type;

			// Skip empty voxels
			if (voxelType == VoxelType::EMPTY) {
				continue;
			}

			// Update face
			if (neighborVoxelType != VoxelType::EMPTY) {
				removedFaces.emplace_back(glm::ivec4{ voxelPos, directionInt });
			}
			else {
				Face face;
				FacePacked::setPosition(face, voxelPos);
				FacePacked::setFace(face, directionInt);
				FacePacked::setTexID(face, static_cast<uint8_t>(voxelType));

				addedFaces.emplace_back(glm::ivec4{ voxelPos, directionInt }, face);
			}
		}
	}

	// Update faces
	{
		std::lock_guard<std::mutex> lock(faceMutex);

		faces.insert(addedFaces.begin(), addedFaces.end());

		for (const auto& faceKey : removedFaces) {
			faces.erase(faceKey);
		}
	}
}
