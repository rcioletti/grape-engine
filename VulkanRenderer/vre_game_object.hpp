#pragma once 

#include "vre_model.hpp"

#include <memory>

namespace vre {

	struct Transform2DComponent {
		glm::vec2 translation{};
		glm::vec2 scale{ 1.f, 1.f };
		float rotation;

		glm::mat2 mat2() {
			const float s = glm::sin(rotation);
			const float c = glm::cos(rotation);
			glm::mat2 rotMatrix{ {c, s}, {-s, c} };

			glm::mat2 scaleMat{ {scale.x, .0f}, {.0f, scale.y} };

			return rotMatrix * scaleMat; 
		}

	};

	class VreGameObject {

	public:
		using id_t = unsigned int;

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
		Transform2DComponent transform2d{};

	private:
		VreGameObject(id_t objId) : id{ objId } {}

		id_t id;
	};
}