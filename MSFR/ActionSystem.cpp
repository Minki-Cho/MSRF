#include "ActionSystem.h"
#include "Input.h"

void ActionSystem::PollFromInput(const Input& input)
{
    fired_.clear();

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