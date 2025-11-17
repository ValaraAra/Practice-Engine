#include "menuScene.h"
#include <imgui.h>

MenuScene::MenuScene(SceneManager& manager) : sceneManager(manager) {}

void MenuScene::gui() {
	ImGui::Begin("Scene List");

	for (const auto& [name, scene] : sceneManager.getScenes()) {
		if (ImGui::Button(name.c_str())) {
			sceneManager.setScene(name);
		}
	}

	ImGui::End();
}
