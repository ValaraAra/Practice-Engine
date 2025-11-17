#pragma once

#include "scene.h"
#include "sceneManager.h"
#include "shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

#pragma once

class SquareScene : public Scene {
public:
	SquareScene(SceneManager& manager);
	~SquareScene();

	//void enter() override;
	//void update() override;
	void render() override;
	void gui() override;
	//void exit() override;

private:
	GLuint VertexArrayObject;
	GLuint VertexBufferObject;

	std::unique_ptr<Shader> shader;
	SceneManager& sceneManager;
};