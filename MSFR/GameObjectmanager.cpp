#include <thread>
#include <sstream>

#include "GameObjectManager.h"
#include "GameObject.h"
#include "Engine.h"
#include "Collision.h"

GameObjectManager::~GameObjectManager()
{
    for (GameObject* objects : gameObjects)
        delete objects;

    gameObjects.clear();

    for (GameObject* obj : pendingAdd)
        delete obj;
    pendingAdd.clear();
}

void GameObjectManager::Add(GameObject* obj)
{
    if (!obj) return;

    std::lock_guard<std::mutex> lock(pendingMutex);
    pendingAdd.push_back(obj);
}

void GameObjectManager::FlushPendingAdds()
{
    // main-thread only
    std::vector<GameObject*> local;
    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        local.swap(pendingAdd);
    }

    for (GameObject* obj : local)
        gameObjects.push_back(obj);
}

void GameObjectManager::Update(double dt)
{
    //Engine::GetLogger().LogEvent("Job workers = " + std::to_string(Engine::GetJobSystem().GetWorkerCount()));

    FlushPendingAdds();

    const bool paused = Engine::GetInput().getPause();
    if (paused)
    {
        // Destroy
        for (auto it = gameObjects.begin(); it != gameObjects.end(); )
        {
            GameObject* obj = *it;
            if (obj && obj->GetDestroyed())
            {
                delete obj;
                it = gameObjects.erase(it);
            }
            else
            {
                ++it;
            }
        }

        FlushPendingAdds();
        return;
    }

    isUpdating.store(true, std::memory_order_release);

    // snapshot
    std::vector<GameObject*> snapshot;
    snapshot.reserve(gameObjects.size());
    for (GameObject* obj : gameObjects)
        snapshot.push_back(obj);

    // parallel update
    auto& js = Engine::GetJobSystem();

    const uint32_t count = static_cast<uint32_t>(snapshot.size());
    js.Dispatch(count, 64, [&](uint32_t i) // use 64 bit for test
        {
            GameObject* obj = snapshot[i];
            if (obj)
                obj->Update(dt);
        });

    js.WaitIdle();

    isUpdating.store(false, std::memory_order_release);

    // main-thread destroy sweep
    for (auto it = gameObjects.begin(); it != gameObjects.end(); )
    {
        GameObject* obj = *it;
        if (obj && obj->GetDestroyed())
        {
            delete obj;
            it = gameObjects.erase(it);
        }
        else
        {
            ++it;
        }
    }

    FlushPendingAdds();
}

void GameObjectManager::DrawAll(mat3<float>& cameraMatrix)
{
    FlushPendingAdds();

    for (GameObject* objects : gameObjects)
        objects->Draw(cameraMatrix);
}

void GameObjectManager::CollideTest()
{
    FlushPendingAdds();

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
                            // log
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
