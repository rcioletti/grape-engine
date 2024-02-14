#pragma once

#include "vre_camera.hpp"

#include <vulkan/vulkan.h>

namespace vre {

	struct FrameInfo {

		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		VreCamera& camera;
	};
}