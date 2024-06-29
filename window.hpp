#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace vraus_VulkanEngine {
	class Window {
	public:
		Window(int w, int h, std::string name);
		~Window();

		Window(const Window&) = delete;
		Window& operator = (const Window&) = delete;

		bool shouldClose() { return glfwWindowShouldClose(window); }
		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
		bool wasWindowResized() const { return frameBufferResized; }
		void resetWindowResizedFlag() { frameBufferResized = false; }

		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
	private:
		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
		void initWindow();

		int width;
		int height;
		bool frameBufferResized = false; // Used as a flag to signal that the window size as changed

		std::string windowName;
		GLFWwindow* window;
	};
}