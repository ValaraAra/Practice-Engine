#include "chunk.h"
#include <tracy/Tracy.hpp>

Chunk::Chunk(VoxelVolume&& data) : voxels(std::move(data.voxels)) {
	voxelCount.store(data.voxelCount);
}

bool Chunk::hasVoxel(const glm::ivec3& chunkPosition, bool ignoreLiquid) const {
	if (!isValidPosition(chunkPosition)) {
		return false;
	}

	std::shared_lock lock(voxelsMutex);

	const VoxelType type = voxels[getVoxelIndex(chunkPosition)].type;
	return type != VoxelType::EMPTY && (!ignoreLiquid || type != VoxelType::LIQUID);
}

std::optional<Voxel> Chunk::getVoxel(const glm::ivec3& position) const {
	if (!isValidPosition(position)) {
		return std::nullopt;
	}

	std::shared_lock lock(voxelsMutex);
	return voxels[getVoxelIndex(position)];
}

bool Chunk::setVoxel(const glm::ivec3& chunkPosition, const VoxelType type, const uint8_t id) {
	if (!isValidPosition(chunkPosition)) {
		return false;
	}

	std::unique_lock lock(voxelsMutex);
	Voxel& voxel = voxels[getVoxelIndex(chunkPosition)];

	// No change
	if ((voxel.type == VoxelType::EMPTY && type == VoxelType::EMPTY) || (voxel.type == type && voxel.id == id)) {
		return false;
	}

	// Update voxel count
	if (type == VoxelType::EMPTY) {
		voxelCount--;
	}
	else if (voxel.type == VoxelType::EMPTY) {
		voxelCount++;
	}

	// Set voxel
	voxel.type = type;
	voxel.id = id;

	dirty.store(true);
	return true;
}

bool Chunk::setVoxel(const glm::ivec3& chunkPosition, const Voxel newVoxel) {
	return setVoxel(chunkPosition, newVoxel.type, newVoxel.id);
}

std::vector<bool> Chunk::setVoxels(const std::vector<std::pair<glm::ivec3, Voxel>>& newVoxels) {
	std::vector<bool> results;
	std::unique_lock lock(voxelsMutex);

	for (const auto& [position, newVoxel] : newVoxels) {
		results.push_back(setVoxel(position, newVoxel));
	}

	return results;
}

void Chunk::clearVoxels() {
	std::unique_lock lock(voxelsMutex);

	for (int i = 0; i < MAX_VOXELS; i++) {
		voxels[i] = {};
	}

	voxelCount.store(0);
	dirty.store(true);
}

void Chunk::getMasks(Masks& masks) const {
	ZoneScopedN("Build Occupancy Masks");
	std::shared_lock lock(voxelsMutex);

	masks.opaque.fill(0);
	masks.liquid.fill(0);
	masks.filled.fill(0);

	for (int y = 0; y < MAX_HEIGHT; y++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			uint32_t maskOpaque = 0;
			uint32_t maskLiquid = 0;

			for (int x = 0; x < CHUNK_SIZE; x++) {
				const int index = x + y * CHUNK_SIZE + z * CHUNK_SIZE * MAX_HEIGHT;
				const Voxel& voxel = voxels[index];

				switch (voxel.type) {
					case VoxelType::EMPTY:
						break;
					case VoxelType::LIQUID:
						maskLiquid |= (1u << x);
						break;
					default: {
						if (VoxelsByType[static_cast<size_t>(voxel.type)][voxel.id].color.a == 255) {
							maskOpaque |= (1u << x);
						}
					}
				}
			}

			masks.opaque[y * CHUNK_SIZE + z] = maskOpaque;
			masks.liquid[y * CHUNK_SIZE + z] = maskLiquid;
			masks.filled[y * CHUNK_SIZE + z] = maskOpaque | maskLiquid;
		}
	}
}

uint32_t Chunk::getMask(const int y, const int z, bool liquid) const {
	std::shared_lock lock(voxelsMutex);

	uint32_t mask = 0;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		const int index = x + y * CHUNK_SIZE + z * CHUNK_SIZE * MAX_HEIGHT;
		const Voxel& voxel = voxels[index];

		if (liquid ? (voxel.type != VoxelType::EMPTY) : VoxelsByType[static_cast<size_t>(voxel.type)][voxel.id].color.a == 255) {
			mask |= (1u << x);
		}
	}

	return mask;
}
