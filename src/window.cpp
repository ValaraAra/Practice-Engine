#include "window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>

static void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	}
	
	std::cout << std::endl;

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	}
	
	std::cout << std::endl;

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	}

	std::cout << std::endl << std::endl;
}

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
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

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

		// Enable OpenGL debug output
		int flags;
		glGetIntegerv(GL_CONTEXT_FLAGS, &flags);

		if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(glDebugOutput, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

			std::cout << "OpenGL debug output enabled" << std::endl;
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