#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

class Window {
public:
	Window();
	~Window();
	void pollEvents();
	void swapBuffers();
	bool shouldClose() const;
	GLFWwindow* getWindow() const;
	void setFullscreen(bool state);

private:
	const glm::ivec2 InitialSize = glm::ivec2(1280, 720);

	GLFWwindow* glfwWindow = nullptr;
	glm::ivec2 windowedSize = glm::ivec2(1280, 720);
	glm::ivec2 windowedPosition = glm::ivec2(0, 0);
	bool fullscreen = false;

	static void keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void framebufferSizeCallbackStatic(GLFWwindow* window, int width, int height);

	void keyCallback(int key, int scancode, int action, int mods);
	void framebufferSizeCallback(int width, int height);
};