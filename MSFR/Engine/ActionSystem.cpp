#include "ActionSystem.h"
#include "Input.h"

void ActionSystem::PollFromInput(const Input& input)
{
    fired_.clear();


    if (input.IsKeyDown(InputKey::Keyboard::Up) || input.IsKeyDown(InputKey::Keyboard::W))
        fired_.push_back(ActionId::Up);

    if (input.IsKeyDown(InputKey::Keyboard::Down) || input.IsKeyDown(InputKey::Keyboard::S))
        fired_.push_back(ActionId::Down);

    if (input.IsKeyDown(InputKey::Keyboard::Left) || input.IsKeyDown(InputKey::Keyboard::A))
        fired_.push_back(ActionId::Left);

    if (input.IsKeyDown(InputKey::Keyboard::Right) || input.IsKeyDown(InputKey::Keyboard::D))
        fired_.push_back(ActionId::Right);

    // === Splash Skip Mapping ===
    // Enter Released
    if (input.IsKeyReleased(InputKey::Keyboard::Enter))
        fired_.push_back(ActionId::Skip);

    // Space Released
    if (input.IsKeyReleased(InputKey::Keyboard::Space))
        fired_.push_back(ActionId::Skip);

    // Mouse Left Released
    if (input.GetMouseReleasedThisFrame())
        fired_.push_back(ActionId::Skip);

}

bool ActionSystem::Has(ActionId action) const
{
    return std::find(fired_.begin(), fired_.end(), action) != fired_.end();
}
