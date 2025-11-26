#pragma once

#include "scene.h"
#include <string>
#include <memory>
#include <unordered_map>

class SceneManager {
public:
	SceneManager();
	~SceneManager();

	void registerScene(const std::string& name, std::unique_ptr<Scene> scene);
	void setScene(const std::string& name);

	const std::unordered_map<std::string, std::unique_ptr<Scene>>& getScenes() const;
	std::unordered_map<std::string, std::vector<std::pair<std::string, Scene&>>> getScenesGrouped() const;
	Scene* getCurrentScene() const;
	std::string getCurrentSceneName() const;

private:
	std::unordered_map<std::string, std::unique_ptr<Scene>> scenes;
	Scene* currentScene = nullptr;
	std::string currentSceneName;
};