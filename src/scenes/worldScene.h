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

struct ProfilingInfo {
	std::chrono::microseconds chunkQueueTime = std::chrono::microseconds(0);
	std::chrono::microseconds chunkGenTime = std::chrono::microseconds(0);
	std::chrono::microseconds worldUpdateTime = std::chrono::microseconds(0);
	std::chrono::microseconds worldDrawTime = std::chrono::microseconds(0);
	std::chrono::microseconds renderTime = std::chrono::microseconds(0);

	std::chrono::microseconds maxChunkQueueTime = std::chrono::microseconds(0);
	std::chrono::microseconds maxChunkGenTime = std::chrono::microseconds(0);
	std::chrono::microseconds maxWorldUpdateTime = std::chrono::microseconds(0);
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
	Shader& shaderLit;
	Shader& shaderLightCube;
	Shader& shaderSkybox;
	SceneManager& sceneManager;
	ShaderManager& shaderManager;
	InputManager& inputManager;

	ProfilingInfo profilingInfo;

	std::vector<CallbackHandle> inputCallbackHandles;

	glm::vec3 lightPos = glm::vec3(30.0f, 39.0f, -90.0f);
	glm::vec3 light2Pos = glm::vec3(0.0f, 0.0f, 0.0f);

	glm::vec3 cameraPos = glm::vec3(0.0f, 85.0f, 0.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	float cameraYaw = -90.0f;
	float cameraPitch = 0.0f;

	int renderDistance = 12;

	glm::vec2 lastMousePosition = glm::vec2(0.0f, 0.0f);
	bool firstMousePosition = true;

	bool flashlightEnabled = false;
	bool lightingDisabled = false;
	bool lightingDebugEnabled = false;
	bool cameraMovementDisabled = false;
	bool exitSceneRequested = false;

	struct {
		bool forward = false;
		bool backward = false;
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
		bool sprint = false;
	} movement;

	float speedMultiplier = 1.0f;
	bool wireframeEnabled = false;

	void updateCamera(float deltaTime);
};