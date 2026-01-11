#pragma once
#include <mutex>

namespace he2rt {
	class GpuCrashTracker {
		std::mutex mutex{};

	public:
		GpuCrashTracker();
		~GpuCrashTracker();

	private:
		void OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize);
		void OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize);
		static void GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData);
		static void ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData);
	};
}
