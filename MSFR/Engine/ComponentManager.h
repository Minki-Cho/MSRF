#pragma once
#include <algorithm> //algorithm
#include <vector> //components
#include <memory> //memory

#include "Component.h" //Component

class ComponentManager
{
public:
    ~ComponentManager()
    {
        Clear();
    }
    void UpdateAll(double dt)
    {
        for (Component* component : components)
        {
            component->Update(dt);
        }
    }

    template<typename T>
    T* GetComponent()
    {
        for (Component* component : components)
        {
            T* ptr = dynamic_cast<T*>(component);
            if (ptr != nullptr)
            {
                return ptr;
            }
        }
        return nullptr;
    }

    void AddComponent(Component* component)
    {
        components.push_back(component);
    }

    template<typename T>
    void RemoveComponent()
    {
        auto it = std::find_if(components.begin(), components.end(), [](Component* element) {
            return (dynamic_cast<T*>(element) != nullptr);
            });
        delete* it;
        components.erase(it);
    }

    void Clear()
    {
        for (Component* component : components)
        {
            delete component;
        }
        components.clear();
    }
private:
    std::vector<Component*> components;
};