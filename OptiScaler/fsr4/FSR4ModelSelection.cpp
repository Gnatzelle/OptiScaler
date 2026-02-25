#include "pch.h"
#include "FSR4ModelSelection.h"
#include <scanner/scanner.h>
#include <detours/detours.h>

PFN_getModelBlob FSR4ModelSelection::o_getModelBlob = nullptr;
PFN_createModel FSR4ModelSelection::o_createModelSDK = nullptr;
PFN_createModel FSR4ModelSelection::o_createModelDriver = nullptr;

uint32_t getCorrectedPreset(uint32_t preset)
{
    auto correctedPreset = preset;
    // Fixup for Quality preset sometimes using model 0, sometimes using model 1
    if (State::Instance().currentFeature)
    {
        auto target = State::Instance().currentFeature->TargetWidth();
        auto render = State::Instance().currentFeature->RenderWidth();

        auto ratio = (float) target / (float) render;

        // Include Ultra Quality in the fix as well
        if (preset == 0 && ratio >= 1.29f)
            correctedPreset = 1;
    }

    if (Config::Instance()->Fsr4Model.has_value())
    {
        correctedPreset = Config::Instance()->Fsr4Model.value();
    }

    State::Instance().currentFsr4Model = correctedPreset;

    return correctedPreset;
}

uint64_t FSR4ModelSelection::hkgetModelBlob(uint32_t preset, uint64_t unknown, uint64_t* source, uint64_t* size)
{
    LOG_FUNC();

    preset = getCorrectedPreset(preset);

    auto result = o_getModelBlob(preset, unknown, source, size);

    return result;
}

uint64_t FSR4ModelSelection::hkcreateModelSDK(void* context, uint32_t preset)
{
    LOG_FUNC();

    preset = getCorrectedPreset(preset);

    auto result = o_createModelSDK(context, preset);

    return result;
}

uint64_t FSR4ModelSelection::hkcreateModelDriver(void* context, uint32_t preset)
{
    LOG_FUNC();

    preset = getCorrectedPreset(preset);

    auto result = o_createModelDriver(context, preset);

    return result;
}

void FSR4ModelSelection::Hook(HMODULE module, FSR4Source source)
{
    if (module == nullptr)
        return;

    if (o_getModelBlob)
    {
        LOG_DEBUG("Unhooking old model selection hooks");

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        if (o_getModelBlob != nullptr)
            DetourDetach(&(PVOID&) o_getModelBlob, hkgetModelBlob);

        if (o_createModelSDK && source == FSR4Source::SDK)
            DetourDetach(&(PVOID&) o_createModelSDK, hkcreateModelSDK);
        else if (o_createModelDriver && source == FSR4Source::DriverDll)
            DetourDetach(&(PVOID&) o_createModelDriver, hkcreateModelDriver);

        if (DetourTransactionCommit() == 0)
        {
            LOG_DEBUG("Unhooked old model selection hooks");
            o_getModelBlob = nullptr;

            if (source == FSR4Source::SDK)
                o_createModelSDK = nullptr;
            else if (source == FSR4Source::DriverDll)
                o_createModelDriver = nullptr;
        }
    }

    if (o_getModelBlob == nullptr)
    {
        const char* pattern = "83 F9 05 0F 87";
        o_getModelBlob = (PFN_getModelBlob) scanner::GetAddress(module, pattern);

        if (o_getModelBlob)
        {
            LOG_DEBUG("Hooking model selection o_getModelBlob: {:X}", (uintptr_t) o_getModelBlob);

            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());

            DetourAttach(&(PVOID&) o_getModelBlob, hkgetModelBlob);

            DetourTransactionCommit();
        }
    }

    const char* pattern403 =
        "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 0F 29 B4 24 "
        "? ? ? ? 0F 29 BC 24 ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 44 8B F2";

    if (!o_createModelSDK && source == FSR4Source::SDK)
    {
        // From amd_fidelityfx_upscaler_dx12 4.0.3.604

        o_createModelSDK = (PFN_createModel) scanner::GetAddress(module, pattern403);

        LOG_DEBUG("Hooking model selection, o_createModelSDK: {:X}", (uintptr_t) o_createModelSDK);

        if (o_createModelSDK)
        {
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());

            DetourAttach(&(PVOID&) o_createModelSDK, hkcreateModelSDK);

            DetourTransactionCommit();
        }
        else
            LOG_ERROR("Couldn't hook model selection");
    }
    else if (!o_createModelDriver && source == FSR4Source::DriverDll)
    {
        o_createModelDriver = (PFN_createModel) scanner::GetAddress(module, pattern403);

        if (!o_createModelDriver)
        {
            // From amdxcffx64 2.1.0.968
            const char* pattern = "48 8B C4 48 89 58 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? "
                                  "? ? 0F 29 70 ? 0F 29 78 ? 48 8B 05";
            o_createModelDriver = (PFN_createModel) scanner::GetAddress(module, pattern);
        }

        if (o_createModelDriver)
        {
            LOG_DEBUG("Hooking model selection, o_createModelDriver: {:X}", (uintptr_t) o_createModelDriver);

            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());

            DetourAttach(&(PVOID&) o_createModelDriver, hkcreateModelDriver);

            DetourTransactionCommit();
        }
        else
            LOG_ERROR("Couldn't hook model selection");
    }
    else
    {
        LOG_DEBUG("Didn't rehook");
    }
}
