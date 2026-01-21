#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct Material {
	glm::vec3 ambient = glm::vec3(1.0f);
	glm::vec3 diffuse = glm::vec3(1.0f);
	glm::vec3 specular = glm::vec3(0.5f);
	float shininess = 32.0f;
};

struct DirectLight {
	glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);

	glm::vec3 ambient = glm::vec3(0.1f);
	glm::vec3 diffuse = glm::vec3(0.5f);
	glm::vec3 specular = glm::vec3(0.5f);
};

struct PointLight {
	glm::vec3 position = glm::vec3(0.0f);

	float constant = 1.0f;
	float linear = 0.1f;
	float quadratic = 0.05f;

	glm::vec3 ambient = glm::vec3(0.1f);
	glm::vec3 diffuse = glm::vec3(0.8f);
	glm::vec3 specular = glm::vec3(1.0f);
};

struct SpotLight {
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(1.0f);

	float cutOff = 0.5f;
	float outerCutOff = 0.75f;

	float constant = 1.0f;
	float linear = 0.1f;
	float quadratic = 0.05f;

	glm::vec3 ambient = glm::vec3(0.0f);
	glm::vec3 diffuse = glm::vec3(1.0f);
	glm::vec3 specular = glm::vec3(1.0f);
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 normal;
};

enum class Axis : uint8_t { X, Y, Z };
enum class Direction : uint8_t { NX, PX, NY, PY, NZ, PZ, COUNT };

constexpr Direction DirectionInverted[static_cast<size_t>(Direction::COUNT)] = {
	Direction::PX,
	Direction::NX,
	Direction::PY,
	Direction::NY,
	Direction::PZ,
	Direction::NZ
};

struct DirectionVectors {
	static inline constexpr glm::ivec3 NX{ -1, 0, 0 };
	static inline constexpr glm::ivec3 PX{ 1, 0, 0 };
	static inline constexpr glm::ivec3 NY{ 0, -1, 0 };
	static inline constexpr glm::ivec3 PY{ 0, 1, 0 };
	static inline constexpr glm::ivec3 NZ{ 0, 0, -1 };
	static inline constexpr glm::ivec3 PZ{ 0, 0, 1 };

	static inline constexpr glm::ivec3 arr[6] = { NX, PX, NY, PY, NZ, PZ };
};

struct DirectionVectors2D {
	static inline constexpr glm::ivec2 NX{ -1, 0 };
	static inline constexpr glm::ivec2 PX{ 1, 0 };
	static inline constexpr glm::ivec2 NY{ 0, -1 };
	static inline constexpr glm::ivec2 PY{ 0, 1 };

	static inline constexpr glm::ivec2 arr[4] = { NX, PX, NY, PY };
};

enum class VoxelType : uint8_t {
	EMPTY = 0,
	STONE = 1,
	DIRT = 2,
	GRASS = 3,
	COUNT
};

struct VoxelTypeData {
	glm::vec3 color;
	bool isSolid;
	bool isTransparent;
};

constexpr VoxelTypeData voxelTypeData[static_cast<size_t>(VoxelType::COUNT)] = {
	{ glm::vec3(0.00f, 0.00f, 0.00f), false, true }, // EMPTY
	{ glm::vec3(0.50f, 0.50f, 0.50f), true, false }, // STONE
	{ glm::vec3(0.57f, 0.42f, 0.30f), true, false }, // DIRT
	{ glm::vec3(0.35f, 0.53f, 0.20f), true, false }, // GRASS
};

struct Voxel {
	uint8_t flags = 0;
	VoxelType type = VoxelType::EMPTY;
};

// Only using 6 bits for face exposure flags, could use the other 2 later
namespace VoxelFlags {
	constexpr uint8_t RIGHT_EXPOSED = 1 << 0;
	constexpr uint8_t LEFT_EXPOSED = 1 << 1;
	constexpr uint8_t TOP_EXPOSED = 1 << 2;
	constexpr uint8_t BOTTOM_EXPOSED = 1 << 3;
	constexpr uint8_t FRONT_EXPOSED = 1 << 4;
	constexpr uint8_t BACK_EXPOSED = 1 << 5;

	constexpr uint8_t FACE_FLAGS[6] = {
		RIGHT_EXPOSED,
		LEFT_EXPOSED,
		TOP_EXPOSED,
		BOTTOM_EXPOSED,
		FRONT_EXPOSED,
		BACK_EXPOSED
	};

	inline bool isFaceExposed(uint8_t flags, uint8_t face) {
		return flags & face;
	}

	inline void setFaceExposed(uint8_t& flags, uint8_t face, bool exposed) {
		if (exposed) {
			flags |= face;
		}
		else {
			flags &= ~face;
		}
	}
}

// Temporary hasher to use unordered_set until switch to 3d arrays
struct ivec3Hasher {
	size_t operator()(const glm::ivec3& vec) const noexcept {
		size_t hashX = std::hash<int>{}(vec.x);
		size_t hashY = std::hash<int>{}(vec.y);
		size_t hashZ = std::hash<int>{}(vec.z);

		return hashX ^ (hashY << 1) ^ (hashZ << 2);
	}
};

struct ivec2Hasher {
	size_t operator()(const glm::ivec2& vec) const noexcept {
		size_t hashX = std::hash<int>{}(vec.x);
		size_t hashY = std::hash<int>{}(vec.y);

		return hashX ^ (hashY << 1);
	}
};
