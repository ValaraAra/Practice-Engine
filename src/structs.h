#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include <string>

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

enum class Axis : uint8_t { X, Y, Z };
enum class Direction : uint8_t { PX, NX, PY, NY, PZ, NZ, COUNT };
enum class Direction2D : uint8_t { PX, NX, PZ, NZ, COUNT };

constexpr Direction DirectionInverted[static_cast<size_t>(Direction::COUNT)] = {
	Direction::NX,
	Direction::PX,
	Direction::NY,
	Direction::PY,
	Direction::NZ,
	Direction::PZ
};

constexpr Direction2D Direction2DInverted[static_cast<size_t>(Direction2D::COUNT)] = {
	Direction2D::NX,
	Direction2D::PX,
	Direction2D::NZ,
	Direction2D::PZ
};

struct DirectionVectors {
	static constexpr glm::ivec3 PX{ 1, 0, 0 };
	static constexpr glm::ivec3 NX{ -1, 0, 0 };
	static constexpr glm::ivec3 PY{ 0, 1, 0 };
	static constexpr glm::ivec3 NY{ 0, -1, 0 };
	static constexpr glm::ivec3 PZ{ 0, 0, 1 };
	static constexpr glm::ivec3 NZ{ 0, 0, -1 };

	static constexpr glm::ivec3 arr[6] = { PX, NX, PY, NY, PZ, NZ };
};

struct DirectionVectors2D {
	static constexpr glm::ivec2 PX{ 1, 0 };
	static constexpr glm::ivec2 NX{ -1, 0 };
	static constexpr glm::ivec2 PZ{ 0, 1 };
	static constexpr glm::ivec2 NZ{ 0, -1 };

	static constexpr glm::ivec2 arr[4] = { PX, NX, PZ, NZ };
};

struct VertexOld {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 normal;
};

// Position: 6 bits per axis (64 possible values)
// Position y: 8 bits per axis (256 possible values)
// Face: 3 bits total (8 possible values, 6 faces)
// TexID: x bits (remaining bits, 9 for now)
struct Face {
	uint32_t packed = 0;
};

namespace FacePacked {
	// Bit widths
	constexpr uint8_t POSITION_BITS = 6;
	constexpr uint8_t POSITION_Y_BITS = 8;
	constexpr uint8_t FACE_BITS = 3;
	constexpr uint8_t TEXID_BITS = 9;

	// Bit shifts
	constexpr uint8_t X_SHIFT = 0;
	constexpr uint8_t Y_SHIFT = POSITION_BITS;
	constexpr uint8_t Z_SHIFT = Y_SHIFT + POSITION_Y_BITS;
	constexpr uint8_t FACE_SHIFT = Z_SHIFT + POSITION_BITS;
	constexpr uint8_t TEXID_SHIFT = FACE_SHIFT + FACE_BITS;

	// Bit masks
	constexpr uint32_t POSITION_MASK = (1 << POSITION_BITS) - 1;
	constexpr uint32_t POSITION_Y_MASK = (1 << POSITION_Y_BITS) - 1;
	constexpr uint32_t FACE_MASK = (1 << FACE_BITS) - 1;
	constexpr uint32_t TEXID_MASK = (1 << TEXID_BITS) - 1;

	inline void setPosition(Face& face, const glm::ivec3& position) {
		face.packed &= ~((POSITION_MASK << X_SHIFT) | (POSITION_Y_MASK << Y_SHIFT) | (POSITION_MASK << Z_SHIFT));
		face.packed |= ((position.x & POSITION_MASK) << X_SHIFT);
		face.packed |= ((position.y & POSITION_Y_MASK) << Y_SHIFT);
		face.packed |= ((position.z & POSITION_MASK) << Z_SHIFT);
	}

	inline glm::ivec3 getPosition(const Face& face) {
		return glm::ivec3((face.packed >> X_SHIFT) & POSITION_MASK, (face.packed >> Y_SHIFT) & POSITION_Y_MASK, (face.packed >> Z_SHIFT) & POSITION_MASK);
	}

	inline void setFace(Face& face, const uint8_t faceDirectionID) {
		face.packed &= ~(FACE_MASK << FACE_SHIFT);
		face.packed |= ((faceDirectionID & FACE_MASK) << FACE_SHIFT);
	}

	inline int getFace(const Face& face) {
		return ((face.packed >> FACE_SHIFT) & FACE_MASK);
	}

	inline void setTexID(Face& face, const uint8_t texID) {
		face.packed &= ~(TEXID_MASK << TEXID_SHIFT);
		face.packed |= ((texID & TEXID_MASK) << TEXID_SHIFT);
	}

	inline uint16_t getTexID(const Face& face) {
		return static_cast<uint16_t>((face.packed >> TEXID_SHIFT) & TEXID_MASK);
	}
}

struct Texel {
	GLubyte r;
	GLubyte g;
	GLubyte b;
	GLubyte a;
};

struct Texture {
	std::string name;
	std::vector<Texel> texels;
};

enum class VoxelType : uint8_t {
	EMPTY = 0,
	STONE = 1,
	DIRT = 2,
	GRASS = 3,
	COUNT
};

struct VoxelData {
	const char* name;
	Texel color;
	bool isSolid;
	bool isTransparent;
};

constexpr VoxelData VoxelTypeData[static_cast<size_t>(VoxelType::COUNT)] = {
	{ "Empty", Texel{0, 0, 0, 0}, false, true},				// EMPTY
	{ "Stone", Texel{ 127, 127, 127, 255 }, true, false },	// STONE
	{ "Dirt", Texel{ 145, 107, 76, 255 }, true, false },	// DIRT
	{ "Grass", Texel{ 89, 135, 51, 255 }, true, false },	// GRASS
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

enum class GenerationType {
	Flat,
	Simple,
	Advanced,
};

// Comparator for chunk priority queue
struct ChunkQueueCompare {
	bool operator()(const std::pair<float, glm::ivec2>& a, const std::pair<float, glm::ivec2>& b) const noexcept {
		return a.first < b.first;
	}
};

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
