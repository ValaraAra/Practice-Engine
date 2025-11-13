#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>

std::atomic<bool> running(false);

std::atomic<int> framebufferWidth(800);
std::atomic<int> framebufferHeight(600);
std::atomic<bool> viewportDirty(false);

int windowedWidth = 800, windowedHeight = 600;
int windowedPosX = 100, windowedPosY = 100;
bool isFullscreen = false;

void syncFramebufferSize(int width, int height) {
	framebufferWidth = width;
	framebufferHeight = height;
	viewportDirty = true;
}

void toggleFullscreen(GLFWwindow* window, GLFWmonitor* monitor, const GLFWvidmode* mode) {
	if (!isFullscreen) {
		glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
		glfwGetWindowSize(window, &windowedWidth, &windowedHeight);

		glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
		glfwSetWindowMonitor(window, NULL, 0, 0, mode->width, mode->height, 0);
		syncFramebufferSize(mode->width, mode->height);
	}
	else {
		glfwSetWindowMonitor(window, NULL, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
		glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
		syncFramebufferSize(windowedWidth, windowedHeight);
	}

	isFullscreen = !isFullscreen;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		toggleFullscreen(window, monitor, mode);
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	syncFramebufferSize(width, height);
}

void render(GLFWwindow* window) {
	glfwMakeContextCurrent(window);

	glfwSwapInterval(1);

	while (running)
	{
		if (viewportDirty.load())
		{
			int width = framebufferWidth.load();
			int height = framebufferHeight.load();
			glViewport(0, 0, width, height);
			viewportDirty = false;
		}

		float time = static_cast<float>(glfwGetTime());

		float r = 0.5f * (std::sin(time * 0.5f + 0.0f) + 1.0f);
		float g = 0.5f * (std::sin(time * 0.5f + 2.0f) + 1.0f);
		float b = 0.5f * (std::sin(time * 0.5f + 4.0f) + 1.0f);

		glClearColor(r, g, b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glfwSwapBuffers(window);
	}

	glfwMakeContextCurrent(NULL);
}

int main()
{
	// Initialize GLFW
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	// Configure GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	// Create Window
	GLFWwindow* window = glfwCreateWindow(windowedWidth, windowedHeight, "Voxel Engine", NULL, NULL);

	if (window == NULL) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();

		return -1;
	}

	glfwMakeContextCurrent(window);

	// Load OpenGL functions using GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glfwMakeContextCurrent(NULL);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);

	running = true;
	std::thread renderThread(render, window);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	running = false;
	renderThread.join();

	glfwTerminate();
	return 0;
}

#ifdef _WIN32
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	return main();
}
#endif