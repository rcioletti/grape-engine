#pragma once

#include "vre_device.hpp"

#include <string>
#include <vector>

namespace vre {
	
	struct PipelineConfigInfo {


	};

	class VrePipeline {
	
	public:
		VrePipeline(VreDevice &device, const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo);
		~VrePipeline();

		VrePipeline(const VrePipeline&) = delete;
		void operator=(const VrePipeline&) = delete;

		static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t  width, uint32_t height);

	private:

		static std::vector<char> readFile(const std::string& filepath);
		
		void createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo);

		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		VreDevice& vreDevice;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
}