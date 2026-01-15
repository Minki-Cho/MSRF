#pragma once
#include <list> //gameObjects
#include "Component.h" //Component inheritance
#include "mat3.h"

class GameObject;

class GameObjectManager : public Component
{
public:
	~GameObjectManager();
	void Add(GameObject* obj);
	void Update(double dt) override;
	void DrawAll(mat3<float>& cameraMatrix);
	void CollideTest();
	const std::list<GameObject*>& Objects();

private:
	std::list<GameObject*> gameObjects;
};