#pragma once

#include "structs.h"
#include <glm/vec3.hpp>
#include <atomic>
#include <array>
#include <shared_mutex>
#include <mutex>

class Chunk {
public:
	bool hasVoxel(const glm::ivec3& chunkPosition) const;
	VoxelType getVoxelType(const glm::ivec3& chunkPosition) const;
	void setVoxelType(const glm::ivec3& chunkPosition, const VoxelType type = VoxelType::STONE);
	void clearVoxels();

	bool isGenerated() const { return generated.load(); }
	void setGenerated() { generated.store(true); }
	bool isDirty() const { return dirty.load(); }
	void clearDirty() { dirty.store(false); }

	std::array<Voxel, MAX_VOXELS> getVoxels() const;
	std::array<Voxel, CHUNK_SIZE * MAX_HEIGHT> getBorderVoxels(Direction2D direction) const;

	int getVoxelCount() const { return voxelCount.load(); }
	unsigned int getVersion() const { return version.load(); }

private:
	std::array<Voxel, MAX_VOXELS> voxels;
	mutable std::shared_mutex voxelsMutex;

	std::atomic<int> voxelCount = 0;
	std::atomic<unsigned int> version = 0;

	std::atomic<bool> dirty = false;
	std::atomic<bool> generated = false;

	static bool isValidPosition(const glm::ivec3& chunkPosition) {
		return (chunkPosition.x >= 0 && chunkPosition.x < CHUNK_SIZE && chunkPosition.y >= 0 && chunkPosition.y < MAX_HEIGHT && chunkPosition.z >= 0 && chunkPosition.z < CHUNK_SIZE);
	};

	int getVoxelIndex(const glm::ivec3& chunkPosition) const {
		return chunkPosition.x + chunkPosition.y * CHUNK_SIZE + chunkPosition.z * CHUNK_SIZE * MAX_HEIGHT;
	};
};