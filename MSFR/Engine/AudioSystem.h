#pragma once

#include <filesystem>
#include <memory>

class Logger;

class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    AudioSystem(AudioSystem&&) = delete;
    AudioSystem& operator=(AudioSystem&&) = delete;

    bool Init(Logger& logger);
    void Shutdown();

    bool IsInitialized() const noexcept;
    bool PlayOneShot(const std::filesystem::path& soundPath);
    void StopAllOneShots();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    Logger* loggerRef = nullptr;
};
