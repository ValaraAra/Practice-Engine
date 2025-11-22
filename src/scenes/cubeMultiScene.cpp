#include "cubeMultiScene.h"
#include "shader.h"
#include "renderer.h"
#include "shaderManager.h"
#include "primitives/cube.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

CubeMultiScene::CubeMultiScene(SceneManager& sceneManager, ShaderManager& shaderManager) : sceneManager(sceneManager), shaderManager(shaderManager), cube(std::make_unique<Cube>()) {
	shader = &shaderManager.get("src/shaders/simple.vert.glsl", "src/shaders/simple.frag.glsl");
}

CubeMultiScene::~CubeMultiScene() {

}

void CubeMultiScene::render(Renderer& renderer) {
	renderer.useShader(shader);

	// Transformation matrices
	glm::mat4 view = glm::lookAt(glm::vec3(4, 3, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 projection = renderer.getProjectionMatrix();

	// Draw
	cube->draw(glm::vec3(0.0f, 0.0f, 0.0f), view, projection, *shader);
	cube->draw(glm::vec3(1.0f, 0.0f, 0.0f), view, projection, *shader);
	cube->draw(glm::vec3(0.0f, 0.0f, 1.0f), view, projection, *shader);
	cube->draw(glm::vec3(1.0f, 0.0f, 1.0f), view, projection, *shader);
}

void CubeMultiScene::gui() {
	ImGui::Begin("Controls");
	if (ImGui::Button("Back to Menu")) {
		sceneManager.setScene("Menu");
	}
	ImGui::End();
}
