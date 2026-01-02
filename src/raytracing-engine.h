#pragma once
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <optional>
#include "acceleration-structures.h"
#include "device.h"
#include "scene.h"

namespace he2rt {
	class RaytracingEngine {
		Device& device;
		Scene scene{ device };

	public:
		RaytracingEngine(Device& device);

		void loadScene();
	};
}
