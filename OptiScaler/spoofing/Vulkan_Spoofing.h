#pragma once

#include "SysUtils.h"

#include <vulkan/vulkan.hpp>

#ifdef VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan_win32.h>
#endif

// #define VULKAN_DEBUG_LAYER

class VulkanSpoofing
{
  private:
    static PFN_vkVoidFunction hkvkGetDeviceProcAddr(VkDevice device, const char* pName);
    static PFN_vkVoidFunction hkvkGetInstanceProcAddr(VkInstance instance, const char* pName);

  public:
    static void HookForVulkanSpoofing(HMODULE vulkanModule);
    static void HookForVulkanExtensionSpoofing(HMODULE vulkanModule);
    static void HookForVulkanVRAMSpoofing(HMODULE vulkanModule);
};
