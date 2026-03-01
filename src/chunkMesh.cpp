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
#include <bit>

ChunkMesh::ChunkMesh() : mesh(nullptr) {

}

ChunkMesh::~ChunkMesh() {

}

void ChunkMesh::update() {
	// Upload mesh if ready
	if (meshState.load() == MeshState::HANDOFF) {
		ZoneScopedN("Mesh Upload");

		meshState.store(MeshState::UPLOADING);

		// Create new mesh
		std::lock_guard<std::mutex> lock(faceMutex);
		mesh = std::make_unique<Mesh>(std::move(faces));

		meshState.store(MeshState::READY);
	}
}

void ChunkMesh::draw(const glm::ivec2 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader) {
	ZoneScopedN("Chunk Draw");

	if (mesh) {
		mesh->draw(glm::vec3(offset.x, 0.0f, offset.y), view, projection, shader);
	}
}

void ChunkMesh::buildMasks(const std::array<Voxel, MAX_VOXELS>& voxels) {
	ZoneScopedN("Build Occupancy Masks");

	for (int y = 0; y < MAX_HEIGHT; y++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			uint32_t mask = 0;

			for (int x = 0; x < CHUNK_SIZE; x++) {
				int voxelIndex = getVoxelIndex(glm::ivec3(x, y, z));

				if (voxels[voxelIndex].type != VoxelType::EMPTY) {
					mask |= (1u << x);
				}
			}

			occupancyMasks[y * CHUNK_SIZE + z] = mask;
		}
	}
}

void ChunkMesh::build(const std::shared_ptr<Chunk> chunk, const ChunkNeighbors& neighbors) {
	ZoneScopedN("Start Mask Meshing");

	// Handle empty chunk
	if (chunk->getVoxelCount() <= 0) {
		std::cout << "Chunk is empty, meshing skipped" << std::endl;
		return;
	}

	meshState.store(MeshState::BUILDING);

	// Clear faces
	std::lock_guard<std::mutex> lock(faceMutex);
	faces.clear();

	// Get voxels
	const std::array<Voxel, MAX_VOXELS>& voxels = chunk->getVoxels();

	// Build
	buildMasks(voxels);
	buildFaces(voxels, neighbors);

	meshState.store(MeshState::HANDOFF);
}

void ChunkMesh::addFace(const glm::ivec3 pos, const VoxelType type, const uint8_t face) {
	Face faceCurrent;
	FacePacked::setPosition(faceCurrent, pos);
	FacePacked::setFace(faceCurrent, face);
	FacePacked::setTexID(faceCurrent, static_cast<uint8_t>(type));

	faces.push_back(faceCurrent);
}

void ChunkMesh::emitFaces(uint32_t mask, int y, int z, uint8_t direction, const std::array<Voxel, MAX_VOXELS>& voxels) {
	while (mask) {
		int x = std::countr_zero(mask);
		mask &= mask - 1;

		glm::ivec3 index = glm::ivec3(x, y, z);

		// Really don't like accessing the voxel type here so many times
		addFace(index, voxels[getVoxelIndex(index)].type, direction);
	}
}

void ChunkMesh::buildFaces(const std::array<Voxel, MAX_VOXELS>& voxels, const ChunkNeighbors& neighbors) {
	ZoneScopedN("Mask Meshing");

	const int CHUNK_SIZE_MINUS_ONE = CHUNK_SIZE - 1;
	const int MAX_HEIGHT_MINUS_ONE = MAX_HEIGHT - 1;

	for (int y = 0; y < MAX_HEIGHT; y++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			uint32_t current = occupancyMasks[y * CHUNK_SIZE + z];

			// Skip empty
			if (current == 0) {
				continue;
			}

			// px
			uint32_t px = current & ~(current >> 1);

			if (neighbors.px) {
				bool neighborSolid = neighbors.px->hasVoxel(glm::ivec3(0, y, z));
				if (neighborSolid) {
					px &= ~(1u << CHUNK_SIZE_MINUS_ONE);
				}
			}
			else {
				px |= (current & (1u << CHUNK_SIZE_MINUS_ONE));
			}

			emitFaces(px, y, z, static_cast<uint8_t>(Direction::PX), voxels);

			// nx
			uint32_t nx = current & ~(current << 1);

			if (neighbors.nx) {
				bool neighborSolid = neighbors.nx->hasVoxel(glm::ivec3(CHUNK_SIZE_MINUS_ONE, y, z));
				if (neighborSolid) {
					nx &= ~1u;
				}
			}
			else {
				nx |= (current & 1u);
			}

			emitFaces(nx, y, z, static_cast<uint8_t>(Direction::NX), voxels);

			// pz
			uint32_t pz;

			if (z < CHUNK_SIZE_MINUS_ONE) {
				pz = current & ~occupancyMasks[y * CHUNK_SIZE + z + 1];
			}
			else if (neighbors.pz) {
				pz = current & ~neighbors.pz->getMask(y, 0);
			}
			else {
				pz = current;
			}

			emitFaces(pz, y, z, static_cast<uint8_t>(Direction::PZ), voxels);

			// nz
			uint32_t nz;

			if (z > 0) {
				nz = current & ~occupancyMasks[y * CHUNK_SIZE + z - 1];
			}
			else if (neighbors.nz) {
				nz = current & ~neighbors.nz->getMask(y, CHUNK_SIZE_MINUS_ONE);
			}
			else {
				nz = current;
			}

			emitFaces(nz, y, z, static_cast<uint8_t>(Direction::NZ), voxels);

			// py
			uint32_t py;

			if (y < MAX_HEIGHT_MINUS_ONE) {
				py = current & ~occupancyMasks[(y + 1) * CHUNK_SIZE + z];
			}
			else {
				py = current;
			}

			emitFaces(py, y, z, static_cast<uint8_t>(Direction::PY), voxels);

			// ny
			uint32_t ny;

			if (y > 0) {
				ny = current & ~occupancyMasks[(y - 1) * CHUNK_SIZE + z];
			}
			else {
				ny = current;
			}

			emitFaces(ny, y, z, static_cast<uint8_t>(Direction::NY), voxels);
		}
	}
}
