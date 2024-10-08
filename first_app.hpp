#pragma once

#include "window.hpp"
#include "device.hpp"
#include "model.hpp"
#include "game_object.hpp"
#include "renderer.hpp"

#include <memory>
#include <vector>

namespace vraus_VulkanEngine {
	class FirstApp {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();
	private:
		void loadGameObjects();

		Window window{ WIDTH, HEIGHT, "Vulkan App" };
		Device device{ window };
		Renderer renderer{window, device};

		std::vector<GameObject> gameObjects;
	};
}