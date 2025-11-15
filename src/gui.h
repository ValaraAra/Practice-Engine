#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

class GUI {
public:
	GUI(GLFWwindow* window);
	~GUI();

	void beginFrame();
	void endFrame();

private:

};