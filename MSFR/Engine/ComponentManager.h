#pragma once
#include <algorithm>
#include <memory>
#include <vector>

#include "Component.h"

class ComponentManager
{
public:
    ~ComponentManager() = default;

    void UpdateAll(double dt)
    {
        for (const auto& component : components)
        {
            component->Update(dt);
        }
    }

    template<typename T>
    T* GetComponent()
    {
        for (const auto& component : components)
        {
            T* ptr = dynamic_cast<T*>(component.get());
            if (ptr != nullptr)
            {
                return ptr;
            }
        }
        return nullptr;
    }

    void AddComponent(std::unique_ptr<Component> component)
    {
        if (!component)
            return;
        components.push_back(std::move(component));
    }

    void AddComponent(Component* component)
    {
        AddComponent(std::unique_ptr<Component>(component));
    }

    template<typename T>
    void RemoveComponent()
    {
        auto it = std::find_if(components.begin(), components.end(), [](const std::unique_ptr<Component>& element) {
            return dynamic_cast<T*>(element.get()) != nullptr;
            });

        if (it != components.end())
        {
            components.erase(it);
        }
    }

    void Clear()
    {
        components.clear();
    }

private:
    std::vector<std::unique_ptr<Component>> components;
};
