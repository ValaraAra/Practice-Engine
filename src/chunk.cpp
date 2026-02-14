#include "chunk.h"
#include <shared_mutex>

bool Chunk::hasVoxel(const glm::ivec3& chunkPosition) const {
	std::shared_lock lock(voxelsMutex);
	return isValidPosition(chunkPosition) && voxels[getVoxelIndex(chunkPosition)].type != VoxelType::EMPTY;
}

VoxelType Chunk::getVoxelType(const glm::ivec3& position) const {
	if (!isValidPosition(position)) {
		return VoxelType::EMPTY;
	}
	std::shared_lock lock(voxelsMutex);

	return voxels[getVoxelIndex(position)].type;
}

void Chunk::setVoxelType(const glm::ivec3& chunkPosition, const VoxelType type) {
	if (!isValidPosition(chunkPosition)) {
		return;
	}
	std::unique_lock lock(voxelsMutex);

	Voxel& voxel = voxels[getVoxelIndex(chunkPosition)];

	if (voxel.type == type) {
		return;
	}

	if (type == VoxelType::EMPTY) {
		voxelCount--;
	}
	else if (voxel.type == VoxelType::EMPTY) {
		voxelCount++;
	}

	voxel.type = type;
	dirty.store(true);
	version++;
}

void Chunk::clearVoxels() {
	std::unique_lock lock(voxelsMutex);

	for (int i = 0; i < MAX_VOXELS; i++) {
		voxels[i].type = VoxelType::EMPTY;
	}

	voxelCount.store(0);
	dirty.store(true);
	version++;
}

std::array<Voxel, MAX_VOXELS> Chunk::getVoxels() const {
	return voxels;
}

std::array<Voxel, CHUNK_SIZE * MAX_HEIGHT> Chunk::getBorderVoxels(Direction2D direction) const {
	std::array<Voxel, CHUNK_SIZE * MAX_HEIGHT> borderVoxels;

	uint8_t directionInt = static_cast<uint8_t>(direction);
	const BorderInfo borderInfo = BorderInfoTable[directionInt];

	glm::ivec3 voxelPos(0);
	glm::ivec3 neighborPos(0);

	voxelPos[static_cast<int>(borderInfo.fixedAxis)] = borderInfo.fixedValue;
	neighborPos[static_cast<int>(borderInfo.fixedAxis)] = borderInfo.neighborValue;

	std::shared_lock lock(voxelsMutex);

	for (int y = 0; y < MAX_HEIGHT; y++)
	{
		voxelPos.y = y;
		neighborPos.y = y;

		for (int sweep = 0; sweep < CHUNK_SIZE; sweep++)
		{
			voxelPos[static_cast<int>(borderInfo.updateAxis)] = sweep;
			neighborPos[static_cast<int>(borderInfo.updateAxis)] = sweep;

			borderVoxels[y * CHUNK_SIZE + sweep] = voxels[getVoxelIndex(voxelPos)];
		}
	}

	return borderVoxels;
}

uint32_t Chunk::getMask(const int y, const int z) const {
	std::shared_lock lock(voxelsMutex);

	uint32_t mask = 0;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		int voxelIndex = getVoxelIndex(glm::ivec3(x, y, z));

		if (voxels[voxelIndex].type != VoxelType::EMPTY) {
			mask |= (1u << x);
		}
	}

	return mask;
}
