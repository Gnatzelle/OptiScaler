#pragma once
#include "SysUtils.h"

// #define VULKAN_DEBUG_LAYER

class VulkanHooks
{
  public:
    static void Hook(HMODULE vulkan1);
    static void Unhook();
};
