#include "worldScene.h"
#include "shader.h"
#include "world.h"
#include "renderer.h"
#include "shaderManager.h"
#include "primitives/cube.h"
#include "primitives/cubeMap.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <tracy/Tracy.hpp>

const float CAMERA_SPEED = 5.0f;

// This looks way too messy
WorldScene::WorldScene(SceneManager& sceneManager, ShaderManager& shaderManager, InputManager& inputManager, Window& window)
	: sceneManager(sceneManager), shaderManager(shaderManager), inputManager(inputManager), window(window),
	world(std::make_unique<World>(GenerationType::Simple)), cube(std::make_unique<Cube>()), skybox(std::make_unique<CubeMap>()),
	worldTextureAtlas(std::make_unique<TextureAtlas>(1, 1, 1)),
	shaderLit(shaderManager.get("src/shaders/lit.vert.glsl", "src/shaders/lit.frag.glsl")),
	shaderLightCube(shaderManager.get("src/shaders/lightCube.vert.glsl", "src/shaders/lightCube.frag.glsl")),
	shaderSkybox(shaderManager.get("src/shaders/skybox.vert.glsl", "src/shaders/skybox.frag.glsl")) {
	tag = "Main";

	// Add textures (must match voxel types)
	for (VoxelData voxelData : VoxelTypeData) {
		worldTextureAtlas->addTexture(voxelData.name, voxelData.color);
	}

	worldTextureAtlas->finish();
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
		if (action == InputAction::ToggleFlashlight && pressed) {
			flashlightEnabled = !flashlightEnabled;
		}
		if (action == InputAction::ToggleLighting && pressed) {
			lightingDisabled = !lightingDisabled;
		}
		if (action == InputAction::ToggleDebug && pressed) {
			lightingDebugEnabled = !lightingDebugEnabled;
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
		glm::vec3 direction(0.0f);
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

	// Light movement
	const float radius = 16.0f;
	static float angle = 0.0f;
	angle += deltaTime * 0.5f;

	light2Pos.x = 0.0f + radius * cos(angle);
	light2Pos.z = 0.0f + radius * sin(angle);

	// World generation
	static float accumulatedTime = 0.0f;
	accumulatedTime += deltaTime;

	if (accumulatedTime >= 0.1f) {
		auto startTime = std::chrono::high_resolution_clock::now();
		world->updateGenerationQueue(cameraPos, renderDistance);
		auto endTime = std::chrono::high_resolution_clock::now();

		profilingInfo.chunkQueueTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
		if (profilingInfo.chunkQueueTime > profilingInfo.maxChunkQueueTime) {
			profilingInfo.maxChunkQueueTime = profilingInfo.chunkQueueTime;
		}

		accumulatedTime = 0.0f;
	}
}

void WorldScene::updateCamera(float deltaTime) {
	ZoneScopedN("Update Camera");
	glm::vec3 velocity(0.0f);

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

	// Normalize and apply velocity
	if (glm::length(velocity) > 0.0f) {
		velocity = glm::normalize(velocity);

		float currentSpeedMult = speedMultiplier;
		if (movement.sprint) {
			currentSpeedMult *= 3.0f;
		}

		cameraPos += velocity * CAMERA_SPEED * currentSpeedMult * deltaTime;
	}
}

void WorldScene::render(Renderer& renderer) {
	auto startTime = std::chrono::high_resolution_clock::now();

	// Use texture atlas
	worldTextureAtlas->use();

	// Setup view and projection matrices
	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	renderer.setProjectionSettings(60.0f, 0.1f, 5000.0f);
	glm::mat4 projection = renderer.getProjectionMatrix();

	// Light cube
	renderer.useShader(&shaderLightCube);

	glm::vec3 lightColor = glm::vec3(1.0f);
	glm::vec3 light2Color = glm::vec3(1.0f, 0.7f, 0.0f);

	Material lightCubeMaterial = {
		lightColor,
		lightColor,
		glm::vec3(0.5f),
		32.0f
	};

	Material lightCube2Material = {
		light2Color,
		light2Color,
		glm::vec3(0.5f),
		32.0f
	};

	glm::vec3 lightDirection = glm::vec3(-0.3f, -1.00f, 0.45f);

	DirectLight directLightInfo = {
		glm::vec3(glm::mat3(view) * lightDirection),
		glm::vec3(0.08f, 0.09f, 0.10f),
		glm::vec3(0.2f, 0.14f, 0.07f),
		glm::vec3(0.1f)
	};

	PointLight lightCubeInfo = {
		glm::vec3(view * glm::vec4(lightPos, 1.0)),
		1.0f,
		0.09f,
		0.032f,
		glm::vec3(0.05f),
		glm::vec3(0.8f),
		glm::vec3(1.0f)
	};

	PointLight lightCube2Info = {
		glm::vec3(view * glm::vec4(light2Pos, 1.0)),
		1.0f,
		0.09f,
		0.032f,
		light2Color * glm::vec3(0.05f),
		light2Color * glm::vec3(0.8f),
		light2Color * glm::vec3(1.0f)
	};

	SpotLight spotLightInfo = {
		glm::vec3(view * glm::vec4(cameraPos, 1.0)),
		glm::vec3(glm::mat3(view) * cameraFront),
		glm::cos(glm::radians(12.5f)),
		glm::cos(glm::radians(15.0f)),
		1.0f,
		0.09f,
		0.032f,
		glm::vec3(0.00f),
		glm::vec3(1.0f),
		glm::vec3(1.0f)
	};

	cube->draw(lightPos, view, projection, shaderLightCube, lightCubeMaterial);
	cube->draw(light2Pos, view, projection, shaderLightCube, lightCube2Material);

	// Directional light direction indicator
	if (lightingDebugEnabled) {
		glm::vec3 lightIndicatorPos = cameraPos + glm::normalize(-lightDirection) * 25.0f;
		Material lightIndicatorMaterial = {
			glm::vec3(0.0f),
			glm::vec3(0.0f),
			glm::vec3(0.0f),
			2.0f
		};
		cube->draw(lightIndicatorPos, view, projection, shaderLightCube, lightIndicatorMaterial);
	}

	// Voxel world
	renderer.useShader(&shaderLit);

	// Disable spotlight
	if (!flashlightEnabled) {
		spotLightInfo.ambient = glm::vec3(0.0f);
		spotLightInfo.diffuse = glm::vec3(0.0f);
		spotLightInfo.specular = glm::vec3(0.0f);
	}

	// Set light uniforms
	shaderLit.setUniforms(directLightInfo);
	shaderLit.setUniforms(lightCubeInfo, 0);
	shaderLit.setUniforms(lightCube2Info, 1);
	shaderLit.setUniforms(spotLightInfo, 0);

	Material worldMaterial = {
		glm::vec3(1.0f),
		glm::vec3(1.0f),
		glm::vec3(0.5f),
		4.0f
	};

	// Disable lighting
	if (lightingDisabled) {
		directLightInfo.ambient = glm::vec3(0.0f);
		directLightInfo.diffuse = glm::vec3(0.0f);
		directLightInfo.specular = glm::vec3(0.0f);
		lightCubeInfo.ambient = glm::vec3(0.0f);
		lightCubeInfo.diffuse = glm::vec3(0.0f);
		lightCubeInfo.specular = glm::vec3(0.0f);
		lightCube2Info.ambient = glm::vec3(0.0f);
		lightCube2Info.diffuse = glm::vec3(0.0f);
		lightCube2Info.specular = glm::vec3(0.0f);
		spotLightInfo.ambient = glm::vec3(0.0f);
		spotLightInfo.diffuse = glm::vec3(0.0f);
		spotLightInfo.specular = glm::vec3(0.0f);

		worldMaterial.ambient = glm::vec3(1.5f);
		worldMaterial.diffuse = glm::vec3(0.0f);
		worldMaterial.specular = glm::vec3(0.0f);
	}

	auto worldDrawTimeStart = std::chrono::high_resolution_clock::now();
	world->draw(cameraPos, renderDistance, view, projection, shaderLit, worldMaterial, wireframeEnabled);
	auto worldDrawTimeEnd = std::chrono::high_resolution_clock::now();

	profilingInfo.worldDrawTime = std::chrono::duration_cast<std::chrono::microseconds>(worldDrawTimeEnd - worldDrawTimeStart);
	if (profilingInfo.worldDrawTime > profilingInfo.maxWorldDrawTime) {
		profilingInfo.maxWorldDrawTime = profilingInfo.worldDrawTime;
	}

	// Skybox
	renderer.useShader(&shaderSkybox);

	// Remove translation from the view matrix
	view = glm::mat4(glm::mat3(view));

	// Set skybox uniforms
	shaderSkybox.setUniform("view", view);
	shaderSkybox.setUniform("projection", projection);
	shaderSkybox.setUniform("skybox", 0);

	skybox->draw(view, projection, shaderSkybox);

	auto endTime = std::chrono::high_resolution_clock::now();

	profilingInfo.renderTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
	if (profilingInfo.renderTime > profilingInfo.maxRenderTime) {
		profilingInfo.maxRenderTime = profilingInfo.renderTime;
	}
}

void WorldScene::gui() {
	glm::ivec2 windowSize = window.getSize();

	ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_Always);
	ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
	if (ImGui::Button("Back to Menu")) {
		sceneManager.setScene("Menu");
	}
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(windowSize.x - 400.0f, 50.0f), ImGuiCond_Always);
	ImGui::Begin("Profiling Info", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", cameraPos.x, cameraPos.y, cameraPos.z);
	ImGui::Text("Chunk Queue Time: %.2f ms (Max: %.2f ms)", profilingInfo.chunkQueueTime.count() / 1000.0f, profilingInfo.maxChunkQueueTime.count() / 1000.0f);
	ImGui::Text("Chunk Generation Time: %.2f ms (Max: %.2f ms)", profilingInfo.chunkGenTime.count() / 1000.0f, profilingInfo.maxChunkGenTime.count() / 1000.0f);
	ImGui::Text("World Draw Time: %.2f ms (Max: %.2f ms)", profilingInfo.worldDrawTime.count() / 1000.0f, profilingInfo.maxWorldDrawTime.count() / 1000.0f);
	ImGui::Text("Total Render Time: %.2f ms (Max: %.2f ms)", profilingInfo.renderTime.count() / 1000.0f, profilingInfo.maxRenderTime.count() / 1000.0f);
	ImGui::Text("Total Chunks: %d", world->getChunkCount());
	ImGui::Text("Rendered Chunks: %d", world->getRenderedChunkCount());
	ImGui::SliderFloat("Speed Multiplier", &speedMultiplier, 0.5f, 10.0f);
	ImGui::SliderInt("Render Distance", &renderDistance, 6, 64);
	ImGui::Checkbox("Wireframe Mode", &wireframeEnabled);
	ImGui::End();
}
