#pragma once

#include "scene.h"
#include "window.h"
#include "input.h"
#include "sceneManager.h"
#include "shaderManager.h"
#include "shader.h"
#include "renderer.h"
#include "textureAtlas.h"
#include <vector>
#include <memory>
#include <chrono>

class World;
class Cube;
class CubeMap;

struct MovementState {
	bool forward = false;
	bool backward = false;
	bool left = false;
	bool right = false;
	bool up = false;
	bool down = false;
	bool sprint = false;
};

struct ProfilingInfo {
	std::chrono::microseconds chunkQueueTime = std::chrono::microseconds(0);
	std::chrono::microseconds meshQueueTime = std::chrono::microseconds(0);
	std::chrono::microseconds chunkGenTime = std::chrono::microseconds(0);
	std::chrono::microseconds worldDrawTime = std::chrono::microseconds(0);
	std::chrono::microseconds renderTime = std::chrono::microseconds(0);

	std::chrono::microseconds maxChunkQueueTime = std::chrono::microseconds(0);
	std::chrono::microseconds maxMeshQueueTime = std::chrono::microseconds(0);
	std::chrono::microseconds maxChunkGenTime = std::chrono::microseconds(0);
	std::chrono::microseconds maxWorldDrawTime = std::chrono::microseconds(0);
	std::chrono::microseconds maxRenderTime = std::chrono::microseconds(0);
};

class WorldScene : public Scene {
public:
	WorldScene(SceneManager& sceneManager, ShaderManager& shaderManager, InputManager& inputManager, Window& window);
	~WorldScene();

	void enter() override;
	void update(float deltaTime) override;
	void render(Renderer& renderer) override;
	void gui() override;
	void exit() override;

private:
	std::unique_ptr<World> world;
	std::unique_ptr<Cube> cube;
	std::unique_ptr<CubeMap> skybox;
	std::unique_ptr<TextureAtlas> worldTextureAtlas;

	Window& window;
	Shader& shaderGeometry;
	Shader& shaderLit;
	Shader& shaderUnlit;
	Shader& shaderLightCube;
	Shader& shaderSkybox;
	SceneManager& sceneManager;
	ShaderManager& shaderManager;
	InputManager& inputManager;

	ProfilingInfo profilingInfo;

	std::vector<CallbackHandle> inputCallbackHandles;

	// Mouse
	glm::vec2 lastMousePosition = glm::vec2(0.0f, 0.0f);
	bool firstMousePosition = true;

	// Camera
	glm::vec3 cameraPos = glm::vec3(0.0f, 85.0f, 0.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	float cameraYaw = -90.0f;
	float cameraPitch = 0.0f;

	MovementState movement;

	// Lights
	glm::vec3 lightDirection = glm::vec3(-0.3f, -1.00f, 0.45f);

	glm::vec3 lightPos = glm::vec3(30.0f, 39.0f, -90.0f);
	glm::vec3 light2Pos = glm::vec3(0.0f, 0.0f, 0.0f);

	glm::vec3 lightColor = glm::vec3(1.0f);
	glm::vec3 light2Color = glm::vec3(1.0f, 0.7f, 0.0f);

	Material lightCubeMaterial = { lightColor, lightColor, glm::vec3(0.5f), 32.0f };
	Material lightCube2Material = { light2Color, light2Color, glm::vec3(0.5f), 32.0f };

	Material worldMaterial = { glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(0.5f), 4.0f };

	// Settings and flags
	bool flashlightEnabled = false;
	bool lightingEnabled = true;
	bool lightingDebugEnabled = false;
	bool wireframeEnabled = false;
	bool ssaoEnabled = true;
	bool ssaoBlurEnabled = true;

	int ssaoQuality = 2; // 1-4 (16, 32, 48, 64 samples)
	float ssaoRadius = 1.0f;
	float ssaoBias = 0.025f;
	int ssaoBlurRadius = 1;

	int renderDistance = 12;
	float speedMultiplier = 1.0f;

	bool cameraMovementDisabled = false;
	bool exitSceneRequested = false;

	void updateCamera(float deltaTime);

	void renderGeometry(Renderer& renderer, const glm::mat4& view, const glm::mat4& projection);
	void renderLit(Renderer& renderer, const glm::mat4& view, const glm::mat4& projection, const Material worldMaterial);
	void renderUnlit(Renderer& renderer, const glm::mat4& view, const glm::mat4& projection);
	void renderExtras(Renderer& renderer, const glm::mat4& view, const glm::mat4& projection);
	void renderSkybox(Renderer& renderer, const glm::mat4& view, const glm::mat4& projection);
};