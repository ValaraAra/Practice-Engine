#include "generation.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>

namespace {
	// Heightmap nodes
	static FastNoise::SmartNode<FastNoise::Perlin> fnPerlinHeight = [] {
		auto ptr = FastNoise::New<FastNoise::Perlin>();
		ptr->SetScale(333.0f);
		ptr->SetOutputMin(-0.7f);
		ptr->SetOutputMax(0.7f);
		return ptr;
		}();

	static FastNoise::SmartNode<FastNoise::FractalFBm> fnFractalHeight = [] {
		auto ptr = FastNoise::New<FastNoise::FractalFBm>();
		ptr->SetSource(fnPerlinHeight);
		ptr->SetOctaveCount(5);
		ptr->SetLacunarity(2.5f);
		ptr->SetWeightedStrength(0.4f);
		return ptr;
		}();

	// Density nodes
	static FastNoise::SmartNode<FastNoise::Perlin> fnPerlinDensity = [] {
		auto ptr = FastNoise::New<FastNoise::Perlin>();
		ptr->SetScale(100.0f);
		ptr->SetOutputMin(-1.0f);
		ptr->SetOutputMax(1.0f);
		return ptr;
		}();

	static FastNoise::SmartNode<FastNoise::FractalFBm> fnFractalDensity = [] {
		auto ptr = FastNoise::New<FastNoise::FractalFBm>();
		ptr->SetSource(fnPerlinDensity);
		ptr->SetGain(0.4f);
		ptr->SetWeightedStrength(0.4f);
		ptr->SetOctaveCount(3);
		ptr->SetLacunarity(2.0f);
		return ptr;
		}();

	static bool isValidPoisson(const glm::ivec2& candidate, const glm::ivec2& candidateCell, const std::vector<glm::ivec2>& points, const std::vector<int>& grid, const int radiusSquared, const int gridSize, const int size) {
		if (candidate.x < 0 || candidate.x >= size || candidate.y < 0 || candidate.y >= size) {
			return false;
		}

		if (candidateCell.x < 0 || candidateCell.x >= gridSize || candidateCell.y < 0 || candidateCell.y >= gridSize) {
			return false;
		}

		int startX = std::max(0, candidateCell.x - 2);
		int endX = std::min(gridSize - 1, candidateCell.x + 2);
		int startY = std::max(0, candidateCell.y - 2);
		int endY = std::min(gridSize - 1, candidateCell.y + 2);

		for (int x = startX; x <= endX; x++) {
			for (int y = startY; y <= endY; y++) {
				int pointIndex = grid[x + y * gridSize];

				if (pointIndex == -1) {
					continue;
				}

				const glm::ivec2& point = points[pointIndex];
				const glm::ivec2 diff = candidate - point;
				const int dot = diff.x * diff.x + diff.y * diff.y;

				if (dot < radiusSquared) {
					return false;
				}
			}
		}

		return true;
	}
}

namespace Generation {
	NoiseOutputPtr generateNoise(const glm::ivec2& offset, FastNoise::SmartNode<FastNoise::FractalFBm> noiseNode) {
		NoiseOutputPtr noiseOutput = std::make_shared<std::array<float, CHUNK_SIZE* CHUNK_SIZE>>();
		noiseNode->GenUniformGrid2D(noiseOutput->data(), offset.x * CHUNK_SIZE, offset.y * CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE, 1, 1, 0);

		return noiseOutput;
	}

	// Based on Bridson's algorithm
	std::vector<glm::ivec2> generatePoisson(const int size, const int radius, const int kSamples, std::mt19937& rng) {
		const float cellSize = radius / glm::root_two<float>();
		const int gridSize = static_cast<int>(std::ceil(size / cellSize));
		const int radiusSquared = radius * radius;

		std::vector<int> grid(gridSize * gridSize, -1);
		std::vector<glm::ivec2> points;
		std::vector<glm::ivec2> active;

		// Start in the center
		const glm::ivec2 startPoint(size / 2, size / 2);

		const int startCellX = static_cast<int>(startPoint.x / cellSize);
		const int startCellY = static_cast<int>(startPoint.y / cellSize);
		grid[startCellX + startCellY * gridSize] = 0;

		points.push_back(startPoint);
		active.push_back(startPoint);

		// Generate points
		while (!active.empty()) {
			// Pick random point from active
			std::uniform_int_distribution<int> distActive(0, static_cast<int>(active.size()) - 1);
			const int index = distActive(rng);
			const glm::ivec2 point = active[index];

			// Generate new points (up to k) around it (annulus, [r, 2r])
			std::uniform_real_distribution<float> distFloat(0.0f, 1.0f);
			std::uniform_int_distribution<int> distRange(radius, 2 * radius);

			bool accepted = false;

			// Up to k samples per point
			for (int i = 0; i < kSamples; i++) {
				float angle = distFloat(rng) * glm::two_pi<float>();
				int distance = distRange(rng);

				const glm::ivec2 candidate = point + glm::ivec2(std::cos(angle) * distance, std::sin(angle) * distance);
				const glm::ivec2 candidateCell(static_cast<int>(candidate.x / cellSize), static_cast<int>(candidate.y / cellSize));

				// Check if candidate is valid
				if (!isValidPoisson(candidate, candidateCell, points, grid, radiusSquared, gridSize, size)) {
					continue;
				}

				// Accept candidate
				grid[candidateCell.x + candidateCell.y * gridSize] = static_cast<int>(points.size());

				points.push_back(candidate);
				active.push_back(candidate);

				accepted = true;
				break;
			}

			// Remove from active if no candidates were accepted
			if (!accepted) {
				active.erase(active.begin() + index);
			}
		}

		return points;
	}

	HeightMapPtr generateHeightMap(const glm::ivec2& offset) {
		NoiseOutputPtr noiseOutput = generateNoise(offset, fnFractalHeight);
		auto& noiseOutputRef = *noiseOutput;

		HeightMapPtr heightmap = std::make_shared<std::array<int, CHUNK_SIZE * CHUNK_SIZE>>();
		auto& heightmapRef = *heightmap;

		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				// Get height from noise
				// Should limit height to like half of max height so we have room for the underground!!
				float noiseValue = noiseOutputRef[x + z * CHUNK_SIZE];
				float normalized = glm::clamp((noiseValue + 1.0f) * 0.5f, 0.0f, 1.0f);
				int heightValue = static_cast<int>(normalized * (MAX_HEIGHT - 1));

				int index = x + z * CHUNK_SIZE;
				heightmapRef[index] = heightValue;
			}
		}

		return heightmap;
	}

	std::vector<glm::ivec2> generateTreeMap(const glm::ivec2& offset, const float minDensity, const int radius, const int kSamples) {
		// Density map
		NoiseOutputPtr noiseOutput = generateNoise(offset, fnFractalDensity);
		auto& noiseOutputRef = *noiseOutput;

		// Poisson disc sampling
		// (not really a good choice for this, need to swap out with deterministic cell jittering or something)
		std::mt19937 rng(offset.x * 123 ^ offset.y * 321);
		std::vector<glm::ivec2> poissonPoints = generatePoisson(CHUNK_SIZE, radius, kSamples, rng);

		// Final map
		std::vector<glm::ivec2> treePoints;

		for (const glm::ivec2& point : poissonPoints) {
			int index = point.x + point.y * CHUNK_SIZE;
			float noiseValue = noiseOutputRef[index];

			if (noiseValue > minDensity) {
				treePoints.push_back(point);
			}
		}

		return treePoints;
	}

	VoxelVolumePtr generateFlat() {
		VoxelVolumePtr volume = std::make_shared<VoxelVolume>();

		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				for (int y = 0; y < 5; y++) {
					int index = x + y * CHUNK_SIZE + z * CHUNK_SIZE * MAX_HEIGHT;

					if (y < 3) {
						volume->voxels[index].type = VoxelType::STONE;
					}
					else {
						volume->voxels[index].type = VoxelType::GRASS;
					}
					volume->voxelCount++;
				}
			}
		}

		return volume;
	}

	VoxelVolumePtr generateSimple(const glm::ivec2& offset) {
		VoxelVolumePtr volume = std::make_shared<VoxelVolume>();

		HeightMapPtr heightmap = generateHeightMap(offset);
		auto& heightmapRef = *heightmap;

		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				const int heightValue = heightmapRef[x + z * CHUNK_SIZE];
				const int baseIndex = x + z * CHUNK_SIZE * MAX_HEIGHT;

				int y = heightValue;

				// Water
				if (heightValue < WATER_HEIGHT) {
					// Water (water height to height value)
					for (y = WATER_HEIGHT; y > heightValue; y--) {
						volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::WATER;
						volume->voxelCount++;
					}

					// Sand (height value to 3 blocks under)
					for (; y >= heightValue - 3 && y >= 0; y--) {
						volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::SAND;
						volume->voxelCount++;
					}
				}
				// Beach
				else if (heightValue == WATER_HEIGHT) {
					// Sand (height value to 2 blocks under)
					for (; y >= heightValue - 2 && y >= 0; y--) {
						volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::SAND;
						volume->voxelCount++;
					}
				}
				// Land
				else {
					// Grass (first block only)
					volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::GRASS;
					volume->voxelCount++;
					y--;

					// Dirt (the 3 blocks under grass)
					for (; y >= heightValue - 3 && y >= 0; y--) {
						volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::DIRT;
						volume->voxelCount++;
					}
				}

				// Stone (underground)
				for (; y >= 0; y--) {
					volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::STONE;
					volume->voxelCount++;
				}
			}
		}

		return volume;
	}

	VoxelVolumePtr generateAdvanced(const glm::ivec2& offset) {
		VoxelVolumePtr volume = std::make_shared<VoxelVolume>();

		HeightMapPtr heightmap = generateHeightMap(offset);
		auto& heightmapRef = *heightmap;

		// Terrain Pass
		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				const int heightValue = heightmapRef[x + z * CHUNK_SIZE];
				const int baseIndex = x + z * CHUNK_SIZE * MAX_HEIGHT;

				int y = heightValue;

				// Water
				if (heightValue < WATER_HEIGHT) {
					// Water (water height to height value)
					for (y = WATER_HEIGHT; y > heightValue; y--) {
						volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::WATER;
						volume->voxelCount++;
					}

					// Sand (height value to 3 blocks under)
					for (; y >= heightValue - 3 && y >= 0; y--) {
						volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::SAND;
						volume->voxelCount++;
					}
				}
				// Beach
				else if (heightValue == WATER_HEIGHT) {
					// Sand (height value to 2 blocks under)
					for (; y >= heightValue - 2 && y >= 0; y--) {
						volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::SAND;
						volume->voxelCount++;
					}
				}
				// Land
				else {
					// Grass (first block only)
					volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::GRASS;
					volume->voxelCount++;
					y--;

					// Dirt (the 3 blocks under grass)
					for (; y >= heightValue - 3 && y >= 0; y--) {
						volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::DIRT;
						volume->voxelCount++;
					}
				}

				// Stone (underground)
				for (; y >= 0; y--) {
					volume->voxels[baseIndex + y * CHUNK_SIZE].type = VoxelType::STONE;
					volume->voxelCount++;
				}
			}
		}

		// Generate trees
		std::vector<glm::ivec2> treePoints = generateTreeMap(offset, -1.0f, 10, 10);

		for (const glm::ivec2& point : treePoints) {
			int heightValue = heightmapRef[point.x + point.y * CHUNK_SIZE];
			int baseIndex = point.x + point.y * CHUNK_SIZE * MAX_HEIGHT;
			int index = baseIndex + heightValue * CHUNK_SIZE;

			Voxel topVoxel = volume->voxels[index];

			if (topVoxel.type == VoxelType::GRASS) {
				volume->voxels[index] = Voxel{ VoxelType::WOOD };
			}
			else {
				volume->voxels[index] = Voxel{ VoxelType::ERROR };
			}
		}

		return volume;
	}
}
