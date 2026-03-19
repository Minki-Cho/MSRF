#include "AudioSystem.h"

#include "Logger.h"

#include <string>

#define MINIAUDIO_IMPLEMENTATION
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244) // miniaudio internal narrowing conversions
#endif
#include "../ThirdParty/miniaudio.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

struct AudioSystem::Impl
{
    ma_engine engine{};
    ma_sound_group oneShotGroup{};
    bool initialized = false;
    bool oneShotGroupInitialized = false;
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
    const ma_result groupResult = ma_sound_group_init(&impl->engine, 0, nullptr, &impl->oneShotGroup);
    if (groupResult == MA_SUCCESS)
    {
        impl->oneShotGroupInitialized = true;
    }
    else
    {
        impl->oneShotGroupInitialized = false;
        loggerRef->LogWarning("Audio one-shot group init failed (miniaudio result=" + std::to_string(static_cast<int>(groupResult)) + ").");
    }

    loggerRef->LogEvent("Audio initialized (miniaudio).");
    return true;
}

void AudioSystem::Shutdown()
{
    if (!impl || !impl->initialized)
    {
        return;
    }

    StopAllOneShots();
    if (impl->oneShotGroupInitialized)
    {
        ma_sound_group_uninit(&impl->oneShotGroup);
        impl->oneShotGroupInitialized = false;
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
    ma_sound_group* group = impl->oneShotGroupInitialized ? &impl->oneShotGroup : nullptr;
    const ma_result result = ma_engine_play_sound(&impl->engine, path.c_str(), group);
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

void AudioSystem::StopAllOneShots()
{
    if (!impl || !impl->initialized || !impl->oneShotGroupInitialized)
    {
        return;
    }

    const ma_result result = ma_sound_group_stop(&impl->oneShotGroup);
    if (result != MA_SUCCESS && loggerRef)
    {
        loggerRef->LogWarning("StopAllOneShots failed (miniaudio result=" + std::to_string(static_cast<int>(result)) + ").");
    }
}
