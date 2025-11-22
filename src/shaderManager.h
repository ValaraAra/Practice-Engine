#pragma once

#include "shader.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>

class ShaderManager {
public:
	Shader& get(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);

private:
	Shader& load(const std::string& key, const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
	Shader* retrieve(const std::string& key);

	std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
};