#include "sceneManager.h"
#include "scene.h"
#include <stdexcept>
#include <iostream>

SceneManager::SceneManager() {

}

SceneManager::~SceneManager() {
	if (currentScene) {
		currentScene->exit();
	}
}

// Registers a scene with a given name, transferring ownership of the scene to the SceneManager
void SceneManager::registerScene(const std::string& name, std::unique_ptr<Scene> scene) {
	if (scenes.find(name) != scenes.end()) {
		throw std::runtime_error("SceneManager (registerScene): Scene '" + name + "' already registered.");
	}

	scenes[name] = std::move(scene);

	std::cout << "Registered Scene: " << name << std::endl << std::endl;
}

// Sets the current scene by name, calling exit on the previous scene and enter on the new scene
void SceneManager::setScene(const std::string& name) {
	std::unordered_map<std::string, std::unique_ptr<Scene>>::iterator iterator = scenes.find(name);

	if (iterator == scenes.end()) {
		throw std::runtime_error("SceneManager (setScene): Scene '" + name + "' not found.");
	}

	if (currentScene) {
		currentScene->exit();
	}

	currentScene = iterator->second.get();
	currentSceneName = name;

	std::cout << "Set Scene: " << name << std::endl;

	currentScene->enter();
}

// Returns a const reference to the map of registered scenes
const std::unordered_map<std::string, std::unique_ptr<Scene>>& SceneManager::getScenes() const {
	return scenes;
}

// Returns scenes grouped by tag
std::unordered_map<std::string, std::vector<std::pair<std::string, Scene&>>> SceneManager::getScenesGrouped() const {
	std::unordered_map<std::string, std::vector<std::pair<std::string, Scene&>>> groups;

	for (const auto& [name, scene] : scenes) {
		groups[scene->getTag()].emplace_back(name, *scene);
	}

	return groups;
}

// Returns a pointer to the current scene
Scene* SceneManager::getCurrentScene() const {
	return currentScene;
}

// Returns the name of the current scene
std::string SceneManager::getCurrentSceneName() const {
	return currentSceneName;
}