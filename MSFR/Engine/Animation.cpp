#include <fstream>
#include <memory>

#include "../Engine/Animation.h"
#include "../Engine/Engine.h"

Animation::Animation() : Animation("assets/images/characters/characters/none_front.anm") {}

Animation::Animation(const std::filesystem::path& fileName) : animSequenceIndex(0)
{
    if (fileName.extension() != ".anm")
    {
        throw std::runtime_error("Bad Filetype.  " + fileName.generic_string() + " not a sprite info file (.anm)");
    }

    std::ifstream inFile(fileName);
    if (!inFile.is_open())
    {
        throw std::runtime_error("Failed to load " + fileName.generic_string());
    }

    std::string label;
    while (!inFile.eof())
    {
        inFile >> label;
        if (label == "PlayFrame")
        {
            int frame;
            float targetTime;
            inFile >> frame;
            inFile >> targetTime;

            animations.push_back(std::make_unique<PlayFrame>(frame, targetTime));
        }
        else if (label == "Loop")
        {
            int loopToFrame;
            inFile >> loopToFrame;
            animations.push_back(std::make_unique<Loop>(loopToFrame));
        }
        else if (label == "End")
        {
            animations.push_back(std::make_unique<End>());
        }
        else
        {
            Engine::GetLogger().LogError("Unknown command " + label + " in anm file " + fileName.generic_string());
        }
    }

    ResetAnimation();
}

void Animation::Update(double dt)
{
    if (!currPlayFrameData)
        return;

    currPlayFrameData->Update(dt);
    if (currPlayFrameData->IsFrameDone())
    {
        currPlayFrameData->ResetTime();
        ++animSequenceIndex;

        if (animSequenceIndex < 0 || animations.size() <= static_cast<size_t>(animSequenceIndex))
        {
            isAnimationDone = true;
            return;
        }

        if (animations[animSequenceIndex]->GetType() == Command::PlayFrame)
        {
            currPlayFrameData = static_cast<PlayFrame*>(animations[animSequenceIndex].get());
        }
        else if (animations[animSequenceIndex]->GetType() == Command::Loop)
        {
            Loop* loopData = static_cast<Loop*>(animations[animSequenceIndex].get());
            animSequenceIndex = loopData->GetLoopToIndex();

            if (animSequenceIndex >= 0 && animations.size() > static_cast<size_t>(animSequenceIndex) &&
                animations[animSequenceIndex]->GetType() == Command::PlayFrame)
            {
                currPlayFrameData = static_cast<PlayFrame*>(animations[animSequenceIndex].get());
            }
            else
            {
                Engine::GetLogger().LogError("Loop does not go to PlayFrame");
                ResetAnimation();
            }
        }
        else if (animations[animSequenceIndex]->GetType() == Command::End)
        {
            isAnimationDone = true;
            return;
        }
    }
}

int Animation::GetDisplayFrame()
{
    return currPlayFrameData ? currPlayFrameData->GetFrameNum() : 0;
}

void Animation::ResetAnimation()
{
    animSequenceIndex = 0;
    isAnimationDone = false;

    if (animations.empty())
    {
        currPlayFrameData = nullptr;
        isAnimationDone = true;
        return;
    }

    if (animations[animSequenceIndex]->GetType() == Command::PlayFrame)
    {
        currPlayFrameData = static_cast<PlayFrame*>(animations[animSequenceIndex].get());
        currPlayFrameData->ResetTime();
        return;
    }

    Engine::GetLogger().LogError("Animation start command is not PlayFrame");
    currPlayFrameData = nullptr;
    isAnimationDone = true;
}

bool Animation::IsAnimationDone()
{
    return isAnimationDone;
}

Animation::PlayFrame::PlayFrame(int frame, double duration)
    : frame(frame), targetTime(duration), timer(0)
{
}

void Animation::PlayFrame::Update(double dt)
{
    timer += dt;
}

bool Animation::PlayFrame::IsFrameDone()
{
    return timer >= targetTime;
}

void Animation::PlayFrame::ResetTime()
{
    timer = 0;
}

int Animation::PlayFrame::GetFrameNum()
{
    return frame;
}

Animation::Loop::Loop(int loopToIndex) : loopToIndex(loopToIndex) {}

int Animation::Loop::GetLoopToIndex()
{
    return loopToIndex;
}

