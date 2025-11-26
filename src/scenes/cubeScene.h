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

class CubeScene : public Scene {
public:
	CubeScene(SceneManager& sceneManager, ShaderManager& shaderManager);
	~CubeScene();

	//void enter() override;
	//void update() override;
	void render(Renderer& renderer) override;
	void gui() override;
	//void exit() override;

private:
	GLuint VertexArrayObject;
	GLuint VertexBufferObject;
	GLuint ElementBufferObject;

	Shader& shader;
	SceneManager& sceneManager;
	ShaderManager& shaderManager;
};