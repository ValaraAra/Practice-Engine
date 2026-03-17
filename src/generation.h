# pragma once

#include "structs.h"
#include <array>
#include <memory>
#include <vector>
#include <random>
#include <FastNoise/FastNoise.h>

namespace Generation {
	using NoiseOutputPtr = std::shared_ptr<std::array<float, CHUNK_SIZE* CHUNK_SIZE>>;
	using HeightMapPtr = std::shared_ptr<std::array<int, CHUNK_SIZE* CHUNK_SIZE>>;
	using VoxelVolumePtr = std::shared_ptr<VoxelVolume>;

	VoxelVolumePtr generateFlat();
	VoxelVolumePtr generateSimple(const glm::ivec2& offset);
	VoxelVolumePtr generateAdvanced(const glm::ivec2& offset);

	namespace Poisson {
		std::vector<glm::ivec2> generatePoisson(const int size, const int radius, const int kSamples, std::mt19937& rng);
	}
}