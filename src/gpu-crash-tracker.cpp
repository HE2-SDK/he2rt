#include "gpu-crash-tracker.h"
#include <GFSDK_Aftermath.h>
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

inline std::string AftermathErrorMessage(GFSDK_Aftermath_Result result) {
    switch (result) {
    case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
        return "Unsupported driver version - requires an NVIDIA R495 display driver or newer.";
    default: {
        std::ostringstream oss{};
        oss << "Aftermath Error 0x" << std::hex << result << std::dec;
        return oss.str();
    }
    }
}

#define AFTERMATH_CHECK_ERROR(FC)                                                                   \
[&]() {                                                                                             \
    GFSDK_Aftermath_Result result = FC;                                                             \
    if (!GFSDK_Aftermath_SUCCEED(result)) {                                                         \
        throw std::runtime_error{ AftermathErrorMessage(result) };                                  \
    }                                                                                               \
}()

namespace he2rt {
	GpuCrashTracker::GpuCrashTracker() {
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_EnableGpuCrashDumps(
            GFSDK_Aftermath_Version_API,
            GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
            GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
            GpuCrashDumpCallback,
            ShaderDebugInfoCallback,
            nullptr,
            nullptr,
            this
        ));
	}

    GpuCrashTracker::~GpuCrashTracker() {
        GFSDK_Aftermath_DisableGpuCrashDumps();
    }

    void GpuCrashTracker::OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize) {
        std::lock_guard<std::mutex> lock{ mutex };

        std::ofstream f{ "crash.nv-gpudmp", std::ios::out | std::ios::binary };
        if (f) {
            f.write((const char*)pGpuCrashDump, gpuCrashDumpSize);
        }
    }

    void GpuCrashTracker::OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
    {
        std::lock_guard<std::mutex> lock{ mutex };

        GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier{};
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(
            GFSDK_Aftermath_Version_API,
            pShaderDebugInfo,
            shaderDebugInfoSize,
            &identifier
        ));

        std::ostringstream filePath${};
        filePath$ << "shader-" << identifier.id[0] << "-" << identifier.id[1] << ".nvdbg";

        std::ofstream f{ filePath$.str(), std::ios::out | std::ios::binary };
        if (f) {
            f.write((const char*)pShaderDebugInfo, shaderDebugInfoSize);
        }
    }

    void GpuCrashTracker::GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData) {
        ((GpuCrashTracker*)pUserData)->OnCrashDump(pGpuCrashDump, gpuCrashDumpSize);
    }

    void GpuCrashTracker::ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData) {
        ((GpuCrashTracker*)pUserData)->OnShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
    }
}