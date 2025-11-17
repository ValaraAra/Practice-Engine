#pragma once

#include "scene.h"
#include "sceneManager.h"

#pragma once

class MenuScene : public Scene {
public:
	MenuScene(SceneManager& manager);

	//void enter() override;
	//void update() override;
	//void render() override;
	void gui() override;
	//void exit() override;

private:
	SceneManager& sceneManager;
};