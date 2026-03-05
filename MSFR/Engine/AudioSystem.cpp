#include "AudioSystem.h"

#include "Logger.h"

#include <string>

#define MINIAUDIO_IMPLEMENTATION
#include "../ThirdParty/miniaudio.h"

struct AudioSystem::Impl
{
    ma_engine engine{};
    bool initialized = false;
};

AudioSystem::AudioSystem() = default;
AudioSystem::~AudioSystem()
{
    Shutdown();
}

bool AudioSystem::Init(Logger& logger)
{
    loggerRef = &logger;

    if (!impl)
    {
        impl = std::make_unique<Impl>();
    }

    if (impl->initialized)
    {
        return true;
    }

    const ma_engine_config config = ma_engine_config_init();
    const ma_result result = ma_engine_init(&config, &impl->engine);
    if (result != MA_SUCCESS)
    {
        loggerRef->LogWarning("Audio init failed (miniaudio result=" + std::to_string(static_cast<int>(result)) + ").");
        return false;
    }

    impl->initialized = true;
    loggerRef->LogEvent("Audio initialized (miniaudio).");
    return true;
}

void AudioSystem::Shutdown()
{
    if (!impl || !impl->initialized)
    {
        return;
    }

    ma_engine_uninit(&impl->engine);
    impl->initialized = false;

    if (loggerRef)
    {
        loggerRef->LogEvent("Audio shutdown.");
    }
}

bool AudioSystem::IsInitialized() const noexcept
{
    return impl && impl->initialized;
}

bool AudioSystem::PlayOneShot(const std::filesystem::path& soundPath)
{
    if (!IsInitialized())
    {
        if (loggerRef)
        {
            loggerRef->LogWarning("PlayOneShot ignored because audio is not initialized.");
        }
        return false;
    }

    const std::string path = soundPath.generic_string();
    const ma_result result = ma_engine_play_sound(&impl->engine, path.c_str(), nullptr);
    if (result != MA_SUCCESS)
    {
        if (loggerRef)
        {
            loggerRef->LogWarning("Failed to play sound: " + path + " (miniaudio result=" + std::to_string(static_cast<int>(result)) + ").");
        }
        return false;
    }

    return true;
}
