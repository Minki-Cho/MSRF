#pragma once
#include <filesystem>
#include <memory>
#include <vector>

class Animation
{
public:
    Animation();
    explicit Animation(const std::filesystem::path& fileName);
    ~Animation() = default;

    void Update(double dt);
    int GetDisplayFrame();
    void ResetAnimation();
    bool IsAnimationDone();

private:
    enum class Command
    {
        PlayFrame,
        Loop,
        End,
    };

    class CommandData
    {
    public:
        virtual ~CommandData() = default;
        virtual Command GetType() = 0;
    };

    class PlayFrame : public CommandData
    {
    public:
        PlayFrame(int frame, double duration);
        Command GetType() override { return Command::PlayFrame; }
        void Update(double dt);
        bool IsFrameDone();
        void ResetTime();
        int GetFrameNum();

    private:
        int frame;
        double targetTime;
        double timer;
    };

    class Loop : public CommandData
    {
    public:
        explicit Loop(int loopToIndex);
        Command GetType() override { return Command::Loop; }
        int GetLoopToIndex();

    private:
        int loopToIndex;
    };

    class End : public CommandData
    {
    public:
        Command GetType() override { return Command::End; }
    };

    bool isAnimationDone = false;
    int animSequenceIndex = 0;
    PlayFrame* currPlayFrameData = nullptr;
    std::vector<std::unique_ptr<CommandData>> animations;
};
