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

const float CAMERA_SPEED = 2.5f;

WorldScene::WorldScene(SceneManager& sceneManager, ShaderManager& shaderManager, InputManager& inputManager, Window& window) : sceneManager(sceneManager), shaderManager(shaderManager), inputManager(inputManager), window(window), world(std::make_unique<World>()), shader(shaderManager.get("src/shaders/simple.vert.glsl", "src/shaders/simple.frag.glsl")) {

}

WorldScene::~WorldScene() {
	// Deregister input callbacks
	if (!inputCallbackHandles.empty()) {
		for (const auto& handle : inputCallbackHandles) {
			inputManager.deregisterInputCallback(handle);
		}
		inputCallbackHandles.clear();
	}
}

void WorldScene::exit() {
	window.setCursorMode(false);
	cameraMovementDisabled = true;

	// Deregister input callbacks
	for (const auto& handle : inputCallbackHandles) {
		inputManager.deregisterInputCallback(handle);
	}
	inputCallbackHandles.clear();
}

void WorldScene::enter() {
	window.setCursorMode(true);
	cameraMovementDisabled = false;

	// Controls
	inputCallbackHandles.push_back(inputManager.registerInputCallback([this](InputAction action, bool pressed) {
		if (action == InputAction::MoveForward) {
			movement.forward = pressed;
		}
		if (action == InputAction::MoveBackward) {
			movement.backward = pressed;
		}
		if (action == InputAction::MoveLeft) {
			movement.left = pressed;
		}
		if (action == InputAction::MoveRight) {
			movement.right = pressed;
		}
		if (action == InputAction::MoveUp) {
			movement.up = pressed;
		}
		if (action == InputAction::MoveDown) {
			movement.down = pressed;
		}
		if (action == InputAction::Shift) {
			movement.sprint = pressed;
		}
		if (action == InputAction::Escape && pressed) {
			cameraMovementDisabled = !window.toggleCursorMode();
		}
		if (action == InputAction::Exit && pressed) {
			exitSceneRequested = true;
		}
		})
	);

	// Mouse movement
	inputCallbackHandles.push_back(inputManager.registerMouseCallback([this](double deltaX, double deltaY) {
		// Check if camera movement is disabled
		if (cameraMovementDisabled) {
			return;
		}

		// Apply sensitivity
		const float sensitivity = 0.1f;
		deltaX *= sensitivity;
		deltaY *= sensitivity;

		// Update yaw and pitch
		cameraYaw += static_cast<float>(deltaX);
		cameraPitch += static_cast<float>(deltaY);

		// Constrain pitch
		if (cameraPitch > 89.0f) {
			cameraPitch = 89.0f;
		}
		if (cameraPitch < -89.0f) {
			cameraPitch = -89.0f;
		}

		// Update camera front
		glm::vec3 direction;
		direction.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
		direction.y = sin(glm::radians(cameraPitch));
		direction.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
		cameraFront = glm::normalize(direction);
		})
	);
}

void WorldScene::update(float deltaTime) {
	if (exitSceneRequested) {
		exitSceneRequested = false;
		sceneManager.setScene("Menu");
		return;
	}

	if (!cameraMovementDisabled) {
		updateCamera(deltaTime);
	}
}

void WorldScene::updateCamera(float deltaTime) {
	glm::vec3 velocity(0.0f);

	float speedMultiplier = 1.0f;

	// Calculate velocity
	if (movement.forward) {
		velocity += cameraFront;
	}
	if (movement.backward) {
		velocity -= cameraFront;
	}
	if (movement.left) {
		velocity -= glm::normalize(glm::cross(cameraFront, cameraUp));
	}
	if (movement.right) {
		velocity += glm::normalize(glm::cross(cameraFront, cameraUp));
	}
	if (movement.up) {
		velocity += cameraUp;
	}
	if (movement.down) {
		velocity -= cameraUp;
	}
	if (movement.sprint) {
		speedMultiplier = 2.5f;
	}

	// Normalize and apply velocity
	if (glm::length(velocity) > 0.0f) {
		velocity = glm::normalize(velocity);
		cameraPos += velocity * CAMERA_SPEED * speedMultiplier * deltaTime;
	}
}

void WorldScene::render(Renderer& renderer) {
	renderer.useShader(&shader);

	// View matrix
	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	// Projection matrix
	renderer.setProjectionSettings(60.0f, 0.1f, 1000.0f);
	glm::mat4 projection = renderer.getProjectionMatrix();

	// Draw
	world->draw(cameraPos, 6, view, projection, shader);
}

void WorldScene::gui() {
	ImGui::Begin("Controls");
	if (ImGui::Button("Back to Menu")) {
		sceneManager.setScene("Menu");
	}
	ImGui::End();
}
