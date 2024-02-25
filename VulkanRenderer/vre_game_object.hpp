#pragma once 

#include "vre_model.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>

namespace vre {

	struct TransformComponent {
		glm::vec3 translation{};
		glm::vec3 scale{ 1.f, 1.f, 1.f };
		glm::vec3 rotation{};

        // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
        // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
        // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		glm::mat4 mat4();
		glm::mat3 normalMatrix();
	};

	class VreGameObject {

	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, VreGameObject>;

		static VreGameObject createGameObject() {
			static id_t currentId = 0;
			return VreGameObject{ currentId++ };
		}

		VreGameObject(const VreGameObject&) = delete;
		VreGameObject &operator=(const VreGameObject&) = delete;
		VreGameObject(VreGameObject&&) = default;
		VreGameObject& operator=(VreGameObject&&) = default;

		id_t getId() { return id; }

		std::shared_ptr<VreModel> model{};
		glm::vec3 color{};
		TransformComponent transform{};

	private:
		VreGameObject(id_t objId) : id{ objId } {}

		id_t id;
	};
}