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

	NoiseOutputPtr generateNoise(const glm::ivec2& offset, FastNoise::SmartNode<FastNoise::FractalFBm> noiseNode);
	std::vector<glm::ivec2> generatePoisson(const int size, const int radius, const int kSamples, std::mt19937& rng);

	HeightMapPtr generateHeightMap(const glm::ivec2& offset);
	std::vector<glm::ivec2> generateTreeMap(const glm::ivec2& offset, const int size, const int radius);


	VoxelVolumePtr generateFlat();
	VoxelVolumePtr generateSimple(const glm::ivec2& offset);
	VoxelVolumePtr generateAdvanced(const glm::ivec2& offset);
}