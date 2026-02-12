#pragma once
#include <vector>
#include <algorithm>
#include "ActionId.h"

class Input;

class ActionSystem
{
public:
    void PollFromInput(const Input& input);

    bool Has(ActionId action) const;

    const std::vector<ActionId>& GetFired() const { return fired_; }

private:
    std::vector<ActionId> fired_;
};
