#include "GameObjectManager.h" //GameObjectManager
#include "GameObject.h" //objects
#include "Engine.h" //Getlogger
#include "Collision.h" //CollideTest

GameObjectManager::~GameObjectManager()
{
	for (GameObject* objects : gameObjects)
	{
		delete objects;
		objects = nullptr;
	}
	gameObjects.clear();
}

void GameObjectManager::Add(GameObject* obj)
{
	gameObjects.push_back(obj);
}

void GameObjectManager::Update(double dt)
{
	std::list<GameObject*> DestroyList;
	for (GameObject* objects : gameObjects)
	{
		if (Engine::GetInput().getPause() == false)
			objects->Update(dt);
		if (objects->GetDestroyed() == true)
		{
			DestroyList.push_back(objects);
		}
	}
	for (GameObject* destroyObject : DestroyList)
	{
		gameObjects.remove(destroyObject);
		delete destroyObject;
	}
}

void GameObjectManager::DrawAll(mat3<float>& cameraMatrix)
{
	for (GameObject* objects : gameObjects)
	{
		objects->Draw(cameraMatrix);
	}
}

void GameObjectManager::CollideTest()
{
	for (GameObject* objectA : gameObjects)
	{
		for (GameObject* objectB : gameObjects)
		{
			if (objectA->GetGOComponent<Collision>() != nullptr && objectB->GetGOComponent<Collision>() != nullptr)
			{
				if (objectA->CanCollideWith(objectB->GetObjectType()))
				{
					if (objectA->DoesCollideWith(objectB) == true)
					{
						if (objectA->GetObjectTypeName() != objectB->GetObjectTypeName())
						{
							//Engine::GetLogger().LogEvent("Collision Detected: " + objectA->GetObjectTypeName() + " and " + objectB->GetObjectTypeName());
						}
						objectA->ResolveCollision(objectB);
					}
				}
			}
		}
	}
}

const std::list<GameObject*>& GameObjectManager::Objects()
{
	return gameObjects;
}
