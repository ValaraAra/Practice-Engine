#pragma once

#include "scene.h"
#include "shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

#pragma once

class TestScene : public Scene {
public:
	TestScene();
	~TestScene();

	//void enter() override;
	void update() override;
	void render() override;
	//void gui() override;
	//void exit() override;

private:
	GLuint VertexArrayObject;
	GLuint VertexBufferObject;

	std::unique_ptr<Shader> shader;
};