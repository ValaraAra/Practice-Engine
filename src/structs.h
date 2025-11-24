#pragma once

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

struct Voxel {
	bool present = false;
	glm::vec3 color = glm::vec3(1.0f);
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
