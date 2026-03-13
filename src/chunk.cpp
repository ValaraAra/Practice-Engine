#include "chunk.h"
#include <shared_mutex>
#include <tracy/Tracy.hpp>
#include <FastNoise/FastNoise.h>

Chunk::Chunk(GenerationType generationType, const glm::ivec2& chunkIndex) : voxels{} {
	ZoneScopedN("Generate");

	static FastNoise::SmartNode<FastNoise::Perlin> fnPerlin = [] {
		auto ptr = FastNoise::New<FastNoise::Perlin>();
		ptr->SetScale(333.0f);
		ptr->SetOutputMin(-0.7f);
		ptr->SetOutputMax(0.7f);
		return ptr;
	}();

	static FastNoise::SmartNode<FastNoise::FractalFBm> fnFractal = [] {
		auto ptr = FastNoise::New<FastNoise::FractalFBm>();
		ptr->SetSource(fnPerlin);
		ptr->SetOctaveCount(5);
		ptr->SetLacunarity(2.5f);
		ptr->SetWeightedStrength(0.4f);
		return ptr;
	}();

	switch (generationType) {
		case GenerationType::Flat: {
			for (int x = 0; x < CHUNK_SIZE; x++) {
				for (int z = 0; z < CHUNK_SIZE; z++) {
					for (int y = 0; y < 5; y++) {
						int index = getVoxelIndex(glm::ivec3(x, y, z));

						if (y < 3) {
							voxels[index].type = VoxelType::STONE;
						}
						else {
							voxels[index].type = VoxelType::GRASS;
						}
						voxelCount++;
					}
				}
			}
			break;
		}
		case GenerationType::Simple: {
			std::vector<float> noiseOutput(CHUNK_SIZE * CHUNK_SIZE);

			{
				ZoneScopedN("Noise");

				fnFractal->GenUniformGrid2D(noiseOutput.data(), chunkIndex.x * CHUNK_SIZE, chunkIndex.y * CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE, 1, 1, 0);
			}

			for (int x = 0; x < CHUNK_SIZE; x++) {
				for (int z = 0; z < CHUNK_SIZE; z++) {
					// Get height from noise
					// Should limit height to like half of max height so we have room for the underground!!
					float noiseValue = noiseOutput[x + z * CHUNK_SIZE];
					float normalized = glm::clamp((noiseValue + 1.0f) * 0.5f, 0.0f, 1.0f);
					int heightValue = static_cast<int>(normalized * (MAX_HEIGHT - 1));

					int baseIndex = x + z * CHUNK_SIZE * MAX_HEIGHT;
					int y = heightValue;

					// Water
					if (heightValue < WATER_HEIGHT) {
						// Water (water height to height value)
						for (y = WATER_HEIGHT; y > heightValue; y--) {
							voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::WATER;
							voxelCount++;
						}

						// Sand (height value to 3 blocks under)
						for (; y >= heightValue - 3 && y >= 0; y--) {
							voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::SAND;
							voxelCount++;
						}
					}
					// Beach
					else if (heightValue == WATER_HEIGHT) {
						// Sand (height value to 2 blocks under)
						for (; y >= heightValue - 2 && y >= 0; y--) {
							voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::SAND;
							voxelCount++;
						}
					}
					// Land
					else {
						// Grass (first block only)
						voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::GRASS;
						voxelCount++;
						y--;

						// Dirt (the 3 blocks under grass)
						for (; y >= heightValue - 3 && y >= 0; y--) {
							voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::DIRT;
							voxelCount++;
						}
					}

					// Stone (underground)
					for (; y >= 0; y--) {
						voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::STONE;
						voxelCount++;
					}
				}
			}
			break;
		}
		case GenerationType::Advanced: {
			break;
		}
		default: {
			break;
		}
	}
}

bool Chunk::hasVoxel(const glm::ivec3& chunkPosition, bool ignoreLiquid) const {
	std::shared_lock lock(voxelsMutex);
	const VoxelType type = voxels[getVoxelIndex(chunkPosition)].type;
	return isValidPosition(chunkPosition) && type != VoxelType::EMPTY && (!ignoreLiquid || !VoxelTypeData[static_cast<uint8_t>(type)].isLiquid);
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

uint32_t Chunk::getMask(const int y, const int z, bool liquid) const {
	std::shared_lock lock(voxelsMutex);

	uint32_t mask = 0;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		const VoxelType type = voxels[getVoxelIndex(glm::ivec3(x, y, z))].type;

		if (liquid ? (type != VoxelType::EMPTY) : VoxelTypeData[static_cast<uint8_t>(type)].color.a == 255) {
			mask |= (1u << x);
		}
	}

	return mask;
}
