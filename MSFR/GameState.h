#pragma once
#include <string> //std::string
#include "ComponentManager.h" //compnents

class Component;

class GameState
{
public:
	virtual void Load() = 0;
	virtual void Update(double dt) = 0;
	virtual void Unload() = 0;
	virtual std::string GetName() = 0;
	virtual void Draw() = 0;

	template<typename T>
	T* GetGSComponent() { return components.GetComponent<T>(); }

protected:
	void AddGSComponent(Component* component) { components.AddComponent(component); }
	void UpdateGSComponents(double dt) { components.UpdateAll(dt); }
	template<typename T>
	void RemoveGSComponent() { components.RemoveComponent<T>(); }
	void ClearGSComponent() { components.Clear(); }

private:
	ComponentManager components;
};