#include "window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

Window::Window(const glm::ivec2 size, const char* title) {
	// Initialize GLFW
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialize GLFW");
	}

	try {
		// Configure GLFW
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		#ifdef __APPLE__
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		#endif

		// Create Window
		glfwWindow = glfwCreateWindow(size.x, size.y, title, NULL, NULL);

		if (glfwWindow == NULL) {
			throw std::runtime_error("Failed to create GLFW window");
		}

		glfwMakeContextCurrent(glfwWindow);

		// Load OpenGL functions using GLAD
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			throw std::runtime_error("Failed to initialize GLAD");
		}

		// Register GLFW Callbacks
		glfwSetWindowUserPointer(glfwWindow, this);
		glfwSetKeyCallback(glfwWindow, keyCallbackStatic);
		glfwSetFramebufferSizeCallback(glfwWindow, framebufferSizeCallbackStatic);
	}
	catch (...) {
		glfwTerminate();
		throw;
	}
}

Window::~Window() {
	glfwTerminate();
}

void Window::pollEvents() {
	glfwPollEvents();
}

void Window::swapBuffers() {
	glfwSwapBuffers(glfwWindow);
}

bool Window::shouldClose() const {
	return glfwWindowShouldClose(glfwWindow);
}

GLFWwindow* Window::getWindow() const {
	return glfwWindow;
}

void Window::setFullscreen(bool state) {
	if (state) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		glfwGetWindowPos(glfwWindow, &windowedPosition.x, &windowedPosition.y);
		glfwGetWindowSize(glfwWindow, &windowedSize.x, &windowedSize.y);

		glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED, GLFW_FALSE);
		glfwSetWindowMonitor(glfwWindow, NULL, 0, 0, mode->width, mode->height, 0);
	}
	else {
		glfwSetWindowMonitor(glfwWindow, NULL, windowedPosition.x, windowedPosition.y, windowedSize.x, windowedSize.y, 0);
		glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED, GLFW_TRUE);
	}

	fullscreen = state;
}

void Window::keyCallbackStatic(GLFWwindow* window, int key, int scancode, int action, int mods) {
	Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (instance) {
		instance->keyCallback(key, scancode, action, mods);
	}
}

void Window::framebufferSizeCallbackStatic(GLFWwindow* window, int width, int height) {
	Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (instance) {
		instance->framebufferSizeCallback(width, height);
	}
}

void Window::keyCallback(int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(glfwWindow, true);
	}
	if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
		setFullscreen(!fullscreen);
	}
}

void Window::framebufferSizeCallback(int width, int height) {
	glViewport(0, 0, width, height);
}