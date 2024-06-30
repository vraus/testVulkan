#pragma once

#include "model.hpp"

// std
#include <memory>

namespace vraus_VulkanEngine {

struct Transform2dComponent {
	glm::vec2 translation{}; // Position Offset
	glm::vec2 scale{ 1.f, 1.f };
	float rotation;  // Angles are in radiant, not degrees.

	glm::mat2 mat2() {
		const float s = glm::sin(rotation);
		const float c = glm::cos(rotation);
		glm::mat2 rotMatrix{ {c, s}, {-s, c} };

		// GLM constructor takes columns, not rows !! { scale.x, .0f } is the first column of scaleMat
		glm::mat2 scaleMat{ {scale.x, .0f},{.0f, scale.y} };
		return rotMatrix * scaleMat; // Combination matrix of Rotation and Scale
	}
};

class GameObject {
public:
	using id_t = unsigned int;

	static GameObject createGameObject() {
		static id_t currentId = 0;
		return GameObject{ currentId++ };
	}

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject(GameObject&&) = default;
	GameObject& operator =(GameObject&&) = default;

	id_t getId() const { return id; }

	std::shared_ptr<Model> model{};
	glm::vec3 color{};
	Transform2dComponent transform2d;

private:
	GameObject(id_t objId) : id{ objId }{}

	id_t id;
};
} // namespace vulkan engine