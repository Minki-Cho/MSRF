#pragma once

#include "owner.h"

union SDL_Event;
struct SDL_Window;

class IProgram
{
public:
    constexpr IProgram() noexcept = default;
    virtual ~IProgram() noexcept = default;

    constexpr IProgram(const IProgram&) noexcept = default;
    constexpr IProgram(IProgram&&) noexcept = default;
    IProgram& operator=(const IProgram&) noexcept = default;
    IProgram& operator=(IProgram&&) noexcept = default;

    virtual void Update() = 0;
    virtual void Draw() = 0;
    //virtual void ImGuiDraw() = 0;
    virtual void HandleEvent(SDL_Window& sdl_window, const SDL_Event& event) = 0;
};

util::owner<IProgram*> create_program(int viewport_width, int viewport_height);
