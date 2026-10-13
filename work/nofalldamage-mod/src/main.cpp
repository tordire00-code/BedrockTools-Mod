#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>
#include <pl/memory/Patch.hpp>
#include <pl/memory/Module.hpp>
#include <android/log.h>

#include <cstdint>
#include <string_view>
#include <vector>

#define LOG_TAG "NoFallDamageMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace {
// IMPORTANT: this offset is build-specific and has NOT been independently
// verified here. Confirm it against the exact Minecraft Bedrock version.
constexpr uintptr_t TARGET_OFFSET = 0x0f28e710;

// AArch64 RET instruction, little-endian bytes.
const std::vector<uint8_t> PATCH_BYTES{0xC0, 0x03, 0x5F, 0xD6};

pl::memory::PatchHandle g_Patch;

bool applyPatch() {
    if (g_Patch) {
        LOGI("Patch already active; skipping duplicate apply.");
        return true;
    }

    const auto base = pl::memory::getModuleBaseAddress("libminecraftpe.so");
    if (base == 0) {
        LOGI("Error: libminecraftpe.so base address was not found.");
        return false;
    }

    const uintptr_t targetAddr = base + TARGET_OFFSET;
    LOGI("Attempting patch at address: 0x%llx",
         static_cast<unsigned long long>(targetAddr));

    // These API names/signatures must match the SDK shipped with the user's
    // LeviLauncher/LeviLamina build. Do not assume restore is crash-proof.
    g_Patch = pl::memory::Patch(targetAddr, PATCH_BYTES);

    if (g_Patch) {
        LOGI("No Fall Damage patch enabled.");
        return true;
    }

    LOGI("Patch apply failed.");
    return false;
}

void removePatch() {
    if (g_Patch) {
        g_Patch.restore();
        g_Patch = {};
        LOGI("Patch restore requested.");
    }
}

void onToggle(std::string_view moduleId, bool enabled) {
    (void)moduleId;
    if (enabled) {
        if (!applyPatch()) {
            LOGI("Toggle is ON, but patch could not be applied.");
        }
    } else {
        removePatch();
    }
}
} // namespace

class NoFallDamageMod {
public:
    static NoFallDamageMod& instance() {
        static NoFallDamageMod mod;
        return mod;
    }

    NoFallDamageMod() : mSelf(*ll::mod::NativeMod::current()) {}

    ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load() {
        getSelf().getLogger().info("No Fall Damage Mod loaded.");
        return true;
    }

    bool enable() {
        return pl::modmenu::ModuleBuilder("nofalldamage.toggle", "No Fall Damage")
            .modId(getSelf().getId())
            .description("Attempts to disable player fall damage.")
            .defaultEnabled(false)
            .onToggle(onToggle)
            .registerModule();
    }

    bool disable() {
        removePatch();
        pl::modmenu::unregisterModule("nofalldamage.toggle");
        return true;
    }

    bool unload() {
        removePatch();
        return true;
    }

private:
    ll::mod::NativeMod& mSelf;
};

PL_REGISTER_MOD(NoFallDamageMod, NoFallDamageMod::instance())
