#include <sstream>
#include <thread>

#include "GameObjectManager.h"
#include "Collision.h"
#include "Engine.h"
#include "GameObject.h"

void GameObjectManager::Add(std::unique_ptr<GameObject> obj)
{
    if (!obj)
        return;

    std::lock_guard<std::mutex> lock(pendingMutex);
    pendingAdd.push_back(std::move(obj));
}

void GameObjectManager::Add(GameObject* obj)
{
    Add(std::unique_ptr<GameObject>(obj));
}

void GameObjectManager::FlushPendingAdds()
{
    std::vector<std::unique_ptr<GameObject>> local;
    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        local.swap(pendingAdd);
    }

    for (auto& obj : local)
        gameObjects.push_back(std::move(obj));
}

void GameObjectManager::Update(double dt)
{
    FlushPendingAdds();

    const bool paused = Engine::GetInput().getPause();
    if (paused)
    {
        for (auto it = gameObjects.begin(); it != gameObjects.end();)
        {
            GameObject* obj = it->get();
            if (obj && obj->GetDestroyed())
            {
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

    std::vector<GameObject*> snapshot;
    snapshot.reserve(gameObjects.size());
    for (const auto& obj : gameObjects)
        snapshot.push_back(obj.get());

    auto& js = Engine::GetJobSystem();

    const uint32_t count = static_cast<uint32_t>(snapshot.size());
    js.Dispatch(count, 64, [&](uint32_t i)
        {
            GameObject* obj = snapshot[i];
            if (obj)
                obj->Update(dt);
        });

    js.WaitIdle();

    isUpdating.store(false, std::memory_order_release);

    for (auto it = gameObjects.begin(); it != gameObjects.end();)
    {
        GameObject* obj = it->get();
        if (obj && obj->GetDestroyed())
        {
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

    for (const auto& object : gameObjects)
        object->Draw(cameraMatrix);
}

void GameObjectManager::CollideTest()
{
    FlushPendingAdds();

    for (const auto& objectAOwner : gameObjects)
    {
        GameObject* objectA = objectAOwner.get();
        for (const auto& objectBOwner : gameObjects)
        {
            GameObject* objectB = objectBOwner.get();

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

const std::list<std::unique_ptr<GameObject>>& GameObjectManager::Objects() const
{
    return gameObjects;
}
