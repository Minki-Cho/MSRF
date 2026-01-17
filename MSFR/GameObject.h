#pragma once
#include "Sprite.h" //Sprites
#include "mat3.h" //Matrix
#include "ComponentManager.h" //components

enum class GameObjectType;

class Component;

class GameObject
{
	friend class Sprite;
public:
	GameObject();
	GameObject(vec2 position);
	GameObject(vec2 position, double rotation, vec2 scale);
	virtual ~GameObject();

	virtual void Update(double dt);
	virtual void Draw(mat3<float> cameraMatrix);

	const mat3<float>& GetMatrix();
	const vec2& GetPosition() const;
	const vec2& GetVelocity() const;
	const vec2& GetScale() const;
	double GetRotation() const;
	void SetPosition(vec2 newPosition);

	bool GetDestroyed();
	void SetDestroyed(bool b);

	//collision
	virtual GameObjectType GetObjectType() = 0;
	virtual std::string GetObjectTypeName() = 0;
	virtual bool CanCollideWith(GameObjectType objectBType);
	bool DoesCollideWith(GameObject* objectB);
	bool DoesCollideWith(vec2 point);
	virtual void ResolveCollision(GameObject*);

	template<typename T>
	T* GetGOComponent() { return components.GetComponent<T>(); }

protected:
	class State
	{
	public:
		virtual void Enter(GameObject* object) = 0;
		virtual void Update(GameObject* object, double dt) = 0;
		virtual void TestForExit(GameObject* object) = 0;
		virtual std::string GetName() = 0;
	};
	class State_Nothing : public State
	{
	public:
		void Enter(GameObject*) override {}
		void Update(GameObject*, double) override {}
		void TestForExit(GameObject*) override {}
		std::string GetName() { return ""; }
	};
	State_Nothing state_nothing;
	void ChangeState(State* newState);
	State* currState;

	void UpdatePosition(vec2 adjustPosition);
	void SetVelocity(vec2 newPosition);
	void UpdateVelocity(vec2 adjustPosition);
	void SetScale(vec2 newScale);
	void SetRotation(double newRotationAmount);
	void UpdateRotation(double newRotationAmount);

	void AddGOComponent(Component* component) { components.AddComponent(component); }
	void UpdateGOComponents(double dt) { components.UpdateAll(dt); }
	void ClearGOComponents() { components.Clear(); }
	template<typename T>
	void RemoveGOComponent() { components.RemoveComponent<T>(); }

private:
	mat3<float> objectMatrix;
	bool updateMatrix;

	double rotation;
	vec2 scale;
	vec2 position;
	vec2 velocity;

	bool shouldDestroyed{ false };

	ComponentManager components;
};