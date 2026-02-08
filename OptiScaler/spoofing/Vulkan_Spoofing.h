#pragma once

#include "SysUtils.h"

class VulkanSpoofing
{
  public:
    static void HookForVulkanSpoofing(HMODULE vulkanModule);
    static void HookForVulkanExtensionSpoofing(HMODULE vulkanModule);
    static void HookForVulkanVRAMSpoofing(HMODULE vulkanModule);
};
