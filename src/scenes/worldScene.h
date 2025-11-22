#pragma once

#include "scene.h"
#include "sceneManager.h"
#include "shaderManager.h"
#include "shader.h"
#include "renderer.h"
#include <memory>

class World;

class WorldScene : public Scene {
public:
	WorldScene(SceneManager& sceneManager, ShaderManager& shaderManager);
	~WorldScene();

	//void enter() override;
	//void update() override;
	void render(Renderer& renderer) override;
	void gui() override;
	//void exit() override;

private:
	std::unique_ptr<World> world;

	Shader& shader;
	SceneManager& sceneManager;
	ShaderManager& shaderManager;
};