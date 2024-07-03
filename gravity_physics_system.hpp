#include "first_app.hpp"

#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace vraus_VulkanEngine {
	class GravityPhysicsSystem {
	public:
		GravityPhysicsSystem(float strength) : strengthGravity{strength} {}

		const float strengthGravity;

		// dt stands for delta time. Specifies the amount of time to advance the simulation
		// substeps is how many intervals to divide the forward time step in. More substeps result in a
		// more stable simulation, but takes longer to compute.
		void update(std::vector<GameObject>& objs, float dt, unsigned int substeps = 1) {
			const float stepDelta = dt / substeps;
			for (int i = 0; i < substeps; i++) {
				stepSimulation(objs, stepDelta);
			}
		}

		glm::vec2 computeForce(GameObject& fromObj, GameObject& toObj) const {
			auto offset = fromObj.transform2d.translation - toObj.transform2d.translation;
			float distanceSquared = glm::dot(offset, offset);

			// Just to ensure a 0 return if objects are to close to each other.
			if (glm::abs(distanceSquared) < 1e-10f)
				return { .0f,.0f };
			
			float force = strengthGravity * toObj.rigidBody2d.mass * fromObj.rigidBody2d.mass / distanceSquared;
			return force * offset / glm::sqrt(distanceSquared);
		}

	private:
		void stepSimulation(std::vector<GameObject>& physicsObjs, float dt) {
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
			}

			// This scales the length of the field line based on the log of the length
		}
	};
}