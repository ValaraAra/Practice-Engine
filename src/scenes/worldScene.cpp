#include "worldScene.h"
#include "shader.h"
#include "world.h"
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

WorldScene::WorldScene(SceneManager& sceneManager, ShaderManager& shaderManager) : sceneManager(sceneManager), shaderManager(shaderManager), world(std::make_unique<World>()), shader(shaderManager.get("src/shaders/simple.vert.glsl", "src/shaders/simple.frag.glsl")) {
	// Add a few extra voxels for testing
	world->addVoxel(glm::ivec3(32, 33, 32));
	world->addVoxel(glm::ivec3(32, 34, 32));
}

WorldScene::~WorldScene() {

}

void WorldScene::render(Renderer& renderer) {
	renderer.useShader(&shader);

	// Transformation matrices
	glm::mat4 view = glm::lookAt(glm::vec3(64, 45, 64), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 projection = renderer.getProjectionMatrix();

	// Draw
	world->draw(view, projection, shader);
}

void WorldScene::gui() {
	ImGui::Begin("Controls");
	if (ImGui::Button("Back to Menu")) {
		sceneManager.setScene("Menu");
	}
	ImGui::End();
}
