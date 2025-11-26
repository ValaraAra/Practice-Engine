#include "menuScene.h"
#include <imgui.h>

MenuScene::MenuScene(SceneManager& manager) : sceneManager(manager) {
	tag = "Menu";
}

void MenuScene::gui() {
	ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
	ImGui::Begin("Scene List", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

	auto tagGroups = sceneManager.getScenesGrouped();

	for (const auto& [groupTag, scenes] : tagGroups) {
		const char* label = groupTag.empty() ? "Practice" : groupTag.c_str();

		// Skip this menu tag
		if (label == tag) {
			continue;
		}

		if (ImGui::CollapsingHeader(label)) {
			for (const auto& [name, scene] : scenes) {
				if (ImGui::Button(name.c_str())) {
					sceneManager.setScene(name);
				}
			}
		}
	}

	ImGui::End();
}
