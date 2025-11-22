#pragma once

#include "scene.h"
#include "sceneManager.h"
#include "shaderManager.h"
#include "shader.h"
#include "renderer.h"
#include <memory>

class Cube;

class CubeMultiScene : public Scene {
public:
	CubeMultiScene(SceneManager& sceneManager, ShaderManager& shaderManager);
	~CubeMultiScene();

	//void enter() override;
	//void update() override;
	void render(Renderer& renderer) override;
	void gui() override;
	//void exit() override;

private:
	std::unique_ptr<Cube> cube;
	Shader* shader;
	SceneManager& sceneManager;
	ShaderManager& shaderManager;
};