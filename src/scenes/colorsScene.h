#pragma once

#include "scene.h"
#include "sceneManager.h"
#include "shaderManager.h"
#include "shader.h"
#include "renderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

#pragma once

class ColorsScene : public Scene {
public:
	ColorsScene(SceneManager& sceneManager, ShaderManager& shaderManager);
	~ColorsScene();

	//void enter() override;
	//void update() override;
	void render(Renderer& renderer) override;
	void gui() override;
	//void exit() override;

private:
	GLuint VertexArrayObject;
	GLuint VertexBufferObject;

	Shader& shader;
	SceneManager& sceneManager;
	ShaderManager& shaderManager;
};