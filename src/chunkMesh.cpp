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

void ChunkMesh::update() {
	// Upload mesh if ready
	if (meshStateOpaque.load() == MeshState::HANDOFF) {
		ZoneScopedN("Mesh Upload Opaque");

		// Create new mesh
		std::lock_guard<std::mutex> lock(faceMutexOpaque);
		meshOpaque = std::make_unique<Mesh>(std::move(facesOpaque));

		meshStateOpaque.store(MeshState::READY);
	}

	// Upload liquid mesh if ready
	if (meshStateLiquid.load() == MeshState::HANDOFF) {
		ZoneScopedN("Mesh Upload Liquid");

		// Create new mesh
		std::lock_guard<std::mutex> lock(faceMutexLiquid);
		meshLiquid = std::make_unique<Mesh>(std::move(facesLiquid));

		meshStateLiquid.store(MeshState::READY);
	}
}

void ChunkMesh::drawOpaque(const glm::ivec2 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader) {
	ZoneScopedN("Chunk Draw Opaque");

	if (meshOpaque) {
		meshOpaque->draw(glm::vec3(offset.x, 0.0f, offset.y), view, projection, shader);
	}
}

void ChunkMesh::drawWater(const glm::ivec2 offset, const glm::mat4& view, const glm::mat4& projection, Shader& shader) {
	ZoneScopedN("Chunk Draw Water");

	if (meshLiquid) {
		meshLiquid->draw(glm::vec3(offset.x, 0.0f, offset.y), view, projection, shader);
	}
}

void ChunkMesh::buildMasks(const std::array<Voxel, MAX_VOXELS>& voxels) {
	ZoneScopedN("Build Occupancy Masks");

	for (int y = 0; y < MAX_HEIGHT; y++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			uint32_t maskOpaque = 0;
			uint32_t maskWater = 0;
			uint32_t maskTranslucent = 0;

			for (int x = 0; x < CHUNK_SIZE; x++) {
				const VoxelType type = voxels[getVoxelIndex(glm::ivec3(x, y, z))].type;

				switch (type) {
					case VoxelType::EMPTY:
						break;
					case VoxelType::WATER:
						maskWater |= (1u << x);
						break;
					default: {
						const uint8_t index = static_cast<uint8_t>(type);

						// Opaque
						if (VoxelTypeData[index].color.a == 255) {
							maskOpaque |= (1u << x);
						}
						// Translucent
						else {
							maskTranslucent |= (1u << x);
						}
					}
				}
			}

			occupancyMasksOpaque[y * CHUNK_SIZE + z] = maskOpaque;
			occupancyMasksLiquid[y * CHUNK_SIZE + z] = maskWater;
			occupancyMasksFilled[y * CHUNK_SIZE + z] = maskOpaque | maskWater;
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

	meshStateOpaque.store(MeshState::BUILDING);
	meshStateLiquid.store(MeshState::BUILDING);

	// Clear faces
	std::lock_guard<std::mutex> lock1(faceMutexOpaque);
	facesOpaque.clear();
	std::lock_guard<std::mutex> lock2(faceMutexLiquid);
	facesLiquid.clear();

	// Get voxels
	const std::array<Voxel, MAX_VOXELS>& voxels = chunk->getVoxels();

	// Build
	buildMasks(voxels);
	buildFaces(false, voxels, neighbors);
	buildFaces(true, voxels, neighbors);

	meshStateOpaque.store(MeshState::HANDOFF);
	meshStateLiquid.store(MeshState::HANDOFF);
}

void ChunkMesh::emitFaces(uint32_t mask, int y, int z, uint8_t direction, const std::array<Voxel, MAX_VOXELS>& voxels, const bool liquid) {
	std::vector<Face>& faceVector = liquid ? facesLiquid : facesOpaque;

	while (mask) {
		int x = std::countr_zero(mask);
		mask &= mask - 1;

		glm::ivec3 position = glm::ivec3(x, y, z);

		// Get type
		// Really don't like accessing the voxel type here so many times
		const uint8_t type = static_cast<uint8_t>(voxels[getVoxelIndex(position)].type);

		// Build face
		Face face;
		FacePacked::setPosition(face, position);
		FacePacked::setFace(face, direction);
		FacePacked::setTexID(face, type);

		faceVector.push_back(face);
	}
}

void ChunkMesh::buildFaces(const bool liquid, const std::array<Voxel, MAX_VOXELS>& voxels, const ChunkNeighbors& neighbors) {
	ZoneScopedN("Mask Meshing");

	const std::array<uint32_t, CHUNK_SIZE* MAX_HEIGHT>& masks = liquid ? occupancyMasksLiquid : occupancyMasksOpaque;
	const std::array<uint32_t, CHUNK_SIZE* MAX_HEIGHT>& occlusionMasks = liquid ? occupancyMasksFilled : occupancyMasksOpaque;

	const int CHUNK_SIZE_MINUS_ONE = CHUNK_SIZE - 1;
	const int MAX_HEIGHT_MINUS_ONE = MAX_HEIGHT - 1;

	for (int y = 0; y < MAX_HEIGHT; y++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			const int index = y * CHUNK_SIZE + z;
			const uint32_t current = masks[index];

			// Skip empty
			if (current == 0) {
				continue;
			}

			// px
			uint32_t px = current & ~(occlusionMasks[index] >> 1);

			if (neighbors.px) {
				bool neighborSolid = neighbors.px->hasVoxel(glm::ivec3(0, y, z), !liquid);
				if (neighborSolid) {
					px &= ~(1u << CHUNK_SIZE_MINUS_ONE);
				}
			}
			else {
				px |= (current & (1u << CHUNK_SIZE_MINUS_ONE));
			}

			emitFaces(px, y, z, static_cast<uint8_t>(Direction::PX), voxels, liquid);

			// nx
			uint32_t nx = current & ~(occlusionMasks[index] << 1);

			if (neighbors.nx) {
				bool neighborSolid = neighbors.nx->hasVoxel(glm::ivec3(CHUNK_SIZE_MINUS_ONE, y, z), !liquid);
				if (neighborSolid) {
					nx &= ~1u;
				}
			}
			else {
				nx |= (current & 1u);
			}

			emitFaces(nx, y, z, static_cast<uint8_t>(Direction::NX), voxels, liquid);

			// pz
			uint32_t pz;

			if (z < CHUNK_SIZE_MINUS_ONE) {
				pz = current & ~occlusionMasks[index + 1];
			}
			else if (neighbors.pz) {
				pz = current & ~neighbors.pz->getMask(y, 0, liquid);
			}
			else {
				pz = current;
			}

			emitFaces(pz, y, z, static_cast<uint8_t>(Direction::PZ), voxels, liquid);

			// nz
			uint32_t nz;

			if (z > 0) {
				nz = current & ~occlusionMasks[index - 1];
			}
			else if (neighbors.nz) {
				nz = current & ~neighbors.nz->getMask(y, CHUNK_SIZE_MINUS_ONE, liquid);
			}
			else {
				nz = current;
			}

			emitFaces(nz, y, z, static_cast<uint8_t>(Direction::NZ), voxels, liquid);

			// py
			uint32_t py;

			if (y < MAX_HEIGHT_MINUS_ONE) {
				py = current & ~occlusionMasks[(y + 1) * CHUNK_SIZE + z];
			}
			else {
				py = current;
			}

			emitFaces(py, y, z, static_cast<uint8_t>(Direction::PY), voxels, liquid);

			// ny
			uint32_t ny;

			if (y > 0) {
				ny = current & ~occlusionMasks[(y - 1) * CHUNK_SIZE + z];
			}
			else {
				ny = current;
			}

			emitFaces(ny, y, z, static_cast<uint8_t>(Direction::NY), voxels, liquid);
		}
	}
}
