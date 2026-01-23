#include "shaderManager.h"
#include <iostream>
#include <glad/glad.h>

Shader& ShaderManager::get(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
	std::string key = vertexShaderPath + "|" + fragmentShaderPath;
	std::cout << "Attempting to load shader: " << key << std::endl;

	// Return existing one if possible
	Shader* existingShader = retrieve(key);

	if (existingShader != nullptr) {
		std::cout << "Shader found in cache" << std::endl << std::endl;
		return *existingShader;
	}

	// Create new one if not found
	return load(key, vertexShaderPath, fragmentShaderPath);
}

// Loads a shader from source code and caches it
Shader& ShaderManager::load(const std::string& key, const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
	try {
		std::unique_ptr<Shader> shader = std::make_unique<Shader>(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
		shaders[key] = std::move(shader);

		std::cout << "Shader loaded and cached" << std::endl << std::endl;
		return *shaders[key];
	}
	catch (const std::exception& error) {
		std::cerr << "Failed to load shader: " << error.what() << std::endl;
		throw;
	}
}

// Retrieves a shader from the cache by key
Shader* ShaderManager::retrieve(const std::string& key) {
	auto iterator = shaders.find(key);
	if (iterator != shaders.end()) {
		return iterator->second.get();
	}

	return nullptr;
}
