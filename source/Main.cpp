#pragma comment(lib, "winmm.lib")

#include "plugin.h"

#include <d3d9.h>

#include "FileSystem/Config.h"
#include "FileSystem/Logging.h"

#include "NtSleep/HighQoS.h"
#include "NtSleep/LegacyFrameLimiter.h"

#include "RenderWare/renderware_hook.h"

#include "SAMP/cchat_unofficial.h"
#include "SAMP/samp_hook.h"

using namespace plugin;

class PreciseFramerateLimiter {
    static inline constexpr D3DCOLOR PREFIX_COLOR = 0x88AA62FF;
    static inline constexpr size_t   GTASA_ID3D9DEVICE_ADDRESS = 0xC97C28;

    static inline int                g_FramerateValue = Config::DEFAULT_FRAMERATE;

    static inline LegacyFrameLimiter legacyFrameLimiter{ g_FramerateValue };

    static void WINAPIV newFpsLimitCommand(const char* str)
    {
        auto len = strlen(str);
        if (!len)
        {
            return;
        }

        int framerate;
        try
        {
            framerate = std::stoi(str);
        }
        catch (std::exception const&)
        {
            cchat_unofficial::AddMessage(PREFIX_COLOR, "Framerate limiter v2: valid amounts are 20-100");
            return;
        }

        if (!Config::IsValidFramerateLimit(framerate))
        {
            cchat_unofficial::AddMessage(PREFIX_COLOR, "Framerate limiter v2: valid amounts are 20-100");
            return;
        }

        std::string message("Framerate limiter v2: " + std::to_string(framerate));
        cchat_unofficial::AddMessage(PREFIX_COLOR, message.c_str());
        SetFrameRate(framerate);
    }

    using PVoidFunctionNoArgs = void(__cdecl*)();
    static inline PVoidFunctionNoArgs originalCGameProcessFromSamp = nullptr;

    // Why vendor agnostic? because implementing both AMD's AntiLag 2 or NVIDIA's Reflex looks exactly like this and may be introduced in the future.
    static void __cdecl cGameProcessFromSamp_VendorAgnosticHook() {
        if (g_FramerateValue) {
            // Do Sleep
            legacyFrameLimiter.Wait();
        }
        originalCGameProcessFromSamp();
    }

public:
	PreciseFramerateLimiter() {
        SetHighQoS();
        renderware_hook::ReplaceRwSetRefreshRate();

        Events::initRwEvent += [] {
            SetFrameRate(Config::DEFAULT_FRAMERATE);

            size_t sampBaseAddress = samp_hook::GetBase();
            if (sampBaseAddress)
            {
                samp_hook::RemoveSampLimits();
                samp_hook::ReplaceSampFpsLimitCommand((PBYTE)newFpsLimitCommand);

                size_t cGameProcessFromSamp = samp_hook::FindCGameProcessFromSamp();
                if (!cGameProcessFromSamp) {
                    Logging::WriteLine("Cannot install mod: Couldn't find SA-MP's CGame::Process function. Use an official client! https://github.com/openmultiplayer/launcher/releases/latest");
                    return;
                }
                originalCGameProcessFromSamp = reinterpret_cast<PVoidFunctionNoArgs>(DetourFunction((PBYTE)cGameProcessFromSamp, (PBYTE)cGameProcessFromSamp_VendorAgnosticHook));
            }
            else
            {
                Events::gameProcessEvent.before += [] {
                    if (g_FramerateValue) {
                        // Do Sleep
                        legacyFrameLimiter.Wait();
                    }
                };
            }
        };
	};

    static void SetFrameRate(int framerate)
    {
        if (framerate < Config::MINIMUM_FRAMERATE) {
            framerate = Config::MINIMUM_FRAMERATE;
        }
        else if (framerate > Config::MAXIMUM_FRAMERATE) {
            framerate = Config::MAXIMUM_FRAMERATE;
        }

        g_FramerateValue = framerate;
        legacyFrameLimiter.UpdateFramerateLimit(framerate);
    }
} PreciseFramerateLimiter;

extern "C" __declspec(dllexport) void __cdecl PreciseFramerateLimiter_SetFrameRate(int framerate) {
    PreciseFramerateLimiter::SetFrameRate(framerate);
}
