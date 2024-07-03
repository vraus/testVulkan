#include "first_app.hpp"

#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cassert>
#include <stdexcept>
#include <array>

namespace vraus_VulkanEngine {

	class GravityPhysicsSystem {
	public:
		GravityPhysicsSystem(float strength) : strengthGravity{ strength } {}

		const float strengthGravity;

		// dt stands for delta time. Specifies the amount of time to advance the simulation
		// substeps is how many intervals to divide the forward time step in. More substeps result in a
		// more stable simulation, but takes longer to compute.
		void update(std::vector<GameObject>& objs, float dt, unsigned int substeps = 1) {
			const float stepDelta = dt / substeps;
			for (unsigned int i = 0; i < substeps; i++) {
				stepSimulation(objs, stepDelta);
			}
		}

		glm::vec2 computeForce(const GameObject& fromObj, const GameObject& toObj) const {
			auto offset = fromObj.transform2d.translation - toObj.transform2d.translation;
			float distanceSquared = glm::dot(offset, offset);

			// Just to ensure a 0 return if objects are to close to each other.
			if (glm::abs(distanceSquared) < 1e-10f)
				return { .0f,.0f };

			float force = strengthGravity * toObj.rigidBody2d.mass * fromObj.rigidBody2d.mass / distanceSquared;
			return force * offset / glm::sqrt(distanceSquared);
		}

	private:
		void stepSimulation(std::vector<GameObject>& physicsObjs, float dt) const {
			// Loops through all pairs of objects and applies attractive force between them
			for (auto iterA = physicsObjs.begin(); iterA != physicsObjs.end(); ++iterA) {
				auto& objA = *iterA;
				for (auto iterB = iterA; iterB != physicsObjs.end(); ++iterB) {
					if (iterA == iterB) continue;
					auto& objB = *iterB;

					auto force = computeForce(objA, objB);
					objA.rigidBody2d.velocity += dt * -force / objA.rigidBody2d.mass;
					objB.rigidBody2d.velocity += dt * force / objB.rigidBody2d.mass;
				}
			}

			// Update each objects position based on its final velocity
			for (auto& obj : physicsObjs) {
				obj.transform2d.translation += dt * obj.rigidBody2d.velocity;
			}
		}


	};

	class Vec2FieldSystem {
	public:
		void update(
			const GravityPhysicsSystem& physicsSystem,
			std::vector<GameObject>& physicsObjs,
			std::vector<GameObject>& vectorField
		) {
			// For each field line we calculate the net gravitation force for that point in space.
			for (auto& vf : vectorField) {
				glm::vec2 direction{};
				for (auto& obj : physicsObjs) {
					direction += physicsSystem.computeForce(obj, vf);
				}

				// This scales the length of the field line based on the log of the length.
				vf.transform2d.scale.x = 
					0.005f + 0.045f * glm::clamp(glm::log(glm::length(direction) + 1) / 3.f, 0.f, 1.f);
				vf.transform2d.rotation = atan2(direction.y, direction.x);

			}
		}
	};

	static std::unique_ptr<Model> createSquareModel(Device& device, glm::vec2 offset) {
		std::vector<Model::Vertex> vertices = {
			{{-0.5f, -0.5f}},
			{{0.5f, 0.5f}},
			{{-0.5f, 0.5f}},
			{{-0.5f, -0.5f}},
			{{0.5f, -0.5f}},
			{{0.5f, 0.5f}}, //
		};
		for (auto& v : vertices) {
			v.position += offset;
		}
		return std::make_unique<Model>(device, vertices);
	}

	static std::unique_ptr<Model> createCircleModel(Device& device, unsigned int numSides) {
		std::vector<Model::Vertex> uniqueVertices;
		uniqueVertices.reserve(numSides + 1);
		for (unsigned int i = 0; i < numSides; i++) {
			float angle = i * glm::two_pi<float>() / numSides;
			uniqueVertices.push_back({ {glm::cos(angle), glm::sin(angle)} });
		}
		uniqueVertices.push_back({}); // adds center vertex at 0, 0

		std::vector<Model::Vertex> vertices;
		vertices.reserve(numSides * 3);
		for (size_t i = 0; i < numSides; i++) {
			vertices.push_back(uniqueVertices[i]);
			vertices.push_back(uniqueVertices[(i + 1) % numSides]);
			vertices.push_back(uniqueVertices[numSides]);
		}
		return std::make_unique<Model>(device, vertices);
	}

	FirstApp::FirstApp() {
		loadGameObjects();
	}

	FirstApp::~FirstApp() {}

	void FirstApp::run() {
		// create some models
		std::shared_ptr<Model> squareModel = createSquareModel(
			device,
			{ .5f, .0f }
		); // offset model by .5f so rotation occurs at edge rather than center of square
		std::shared_ptr<Model> circleModel = createCircleModel(device, 64);

		// create physics objects
		std::vector<GameObject> physicsObjects{};
		auto red = GameObject::createGameObject();
		red.transform2d.scale = glm::vec2{ .05f };
		red.transform2d.translation = { .5f, .5f };
		red.color = { 1.f, 0.f, 0.f };
		red.rigidBody2d.velocity = { -.5f, .0f };
		red.model = circleModel;
		physicsObjects.push_back(std::move(red));
		auto blue = GameObject::createGameObject();
		blue.transform2d.scale = glm::vec2{ .05f };
		blue.transform2d.translation = { .45f, .25f };
		blue.color = { 0.f, 0.f, 1.f };
		blue.rigidBody2d.velocity = { -.5f, .0f };
		blue.model = circleModel;
		physicsObjects.push_back(std::move(blue));

		// create vector field
		std::vector<GameObject> vectorField{};
		int gridCount = 40;
		for (int i = 0; i < gridCount; i++) {
			for (int j = 0; j < gridCount; j++) {
				auto vf = GameObject::createGameObject();
				vf.transform2d.scale = glm::vec2(0.005f);
				vf.transform2d.translation = {
					-1.0f + (i + 0.5f) * 2.0f / gridCount,
					-1.0f + (j + 0.5f) * 2.0f / gridCount
				};
				vf.color = glm::vec3(1.0f); // blanc
				vf.model = squareModel;
				vectorField.push_back(std::move(vf));
			}
		}

		GravityPhysicsSystem gravitySystem(0.81f);
		Vec2FieldSystem vecFieldSystem{};
		

		SimpleRenderSystem simpleRenderSystem{ device, renderer.getSwapChainRenderPass() };

		while (!window.shouldClose()) {
			glfwPollEvents();

			if (auto commandBuffer = renderer.beginFrame()) { // beginFrame function returns null if the swapChain needs to be recreated
				// update systems
				gravitySystem.update(physicsObjects, 1.f / 60, 5);
				vecFieldSystem.update(gravitySystem, physicsObjects, vectorField);

				// Example of usage and why we put every steps of drawing a frame appart:
				// Begin offscreen shadow pass
				// Render shadow casting objects
				// End offscreen shadow pass
				
				renderer.beginSwapChainRenderPass(commandBuffer);
				// simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
				simpleRenderSystem.renderGameObjects(commandBuffer, physicsObjects);
				simpleRenderSystem.renderGameObjects(commandBuffer, vectorField);
				renderer.endSwapChainRenderPass(commandBuffer);
				renderer.endFrame();
			}
		}
		vkDeviceWaitIdle(device.device()); // To block the CPU until all GPU operations are completed. We can then safely clean up all resources.
	}

	void FirstApp::loadGameObjects()
	{
		std::vector<Model::Vertex> vertices{
			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};
		auto model = std::make_shared<Model>(device, vertices); // Allows to have ONE model instance used by MULTIPLE game objects
		
		auto triangle = GameObject::createGameObject();
		triangle.model = model;
		triangle.color = { .1f, .8f, .1f };
		triangle.transform2d.translation.x = .2f;
		triangle.transform2d.scale = { 2.f, .5f };
		triangle.transform2d.rotation = .25f * glm::two_pi<float>();

		gameObjects.push_back(std::move(triangle));

		// https://www.color-hex.com/color-palette/5361
		// std::vector<glm::vec3> colors{
		//   {1.f, .7f, .73f},
		//   {1.f, .87f, .73f},
		//   {1.f, 1.f, .73f},
		//   {.73f, 1.f, .8f},
		//   {.73, .88f, 1.f}
		// };
		// for (auto& color : colors) {
		// 	color = glm::pow(color, glm::vec3{ 2.2f });
		// }
		// for (int i = 0; i < 40; i++) {
		// 	auto triangle = GameObject::createGameObject();
		// 	triangle.model = model;
		// 	triangle.transform2d.scale = glm::vec2(.5f) + i * 0.025f;
		// 	triangle.transform2d.rotation = i * glm::pi<float>() * .025f;
		// 	triangle.color = colors[i % colors.size()];
		// 	gameObjects.push_back(std::move(triangle));
		// }
		
	}

}