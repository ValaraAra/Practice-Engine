#pragma once

#include <glm/vec2.hpp>
#include <glm/gtc/noise.hpp>

namespace Noise {
	// Generates 2D Perlin noise with multiple octaves and normalizes the result to [0,1]
	float GenNoise2D(const glm::vec2& position, float baseFrequency, float baseAmplitude, int octaves, float lacunarity, float persistence) {
		float total = 0.0f;
		float frequency = baseFrequency;
		float amplitude = baseAmplitude;
		float maxAmplitude = 0.0f;

		for (int i = 0; i < octaves; ++i) {
			float rawNoise = glm::perlin(position * frequency);
			total += rawNoise * amplitude;
			maxAmplitude += amplitude;

			frequency *= lacunarity;
			amplitude *= persistence;
		}

		if (maxAmplitude <= 0.0f) return 0.0f;

		// Normalize from [-maxAmplitude, maxAmplitude] to [-1,1] to [0,1]
		float normalized = (total / maxAmplitude + 1.0f) * 0.5f;
		if (normalized < 0.0f) normalized = 0.0f;
		if (normalized > 1.0f) normalized = 1.0f;
		return normalized;
	}
}