#include <sstream>
#include <thread>

#include "GameObjectManager.h"
#include "Collision.h"
#include "Engine.h"
#include "GameObject.h"

GameObjectManager::~GameObjectManager() = default;

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
        }, "GameObject.Update");


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
    {
        if (!object || object->GetDestroyed())
            continue;

        object->Draw(cameraMatrix);
    }
}

void GameObjectManager::CollideTest()
{
    FlushPendingAdds();

    std::vector<GameObject*> collidables;
    collidables.reserve(gameObjects.size());

    for (const auto& objectOwner : gameObjects)
    {
        GameObject* object = objectOwner.get();
        if (!object || object->GetDestroyed())
            continue;
        if (object->GetGOComponent<Collision>() == nullptr)
            continue;

        collidables.push_back(object);
    }

    const std::size_t count = collidables.size();
    for (std::size_t i = 0; i < count; ++i)
    {
        GameObject* objectA = collidables[i];
        for (std::size_t j = i + 1; j < count; ++j)
        {
            GameObject* objectB = collidables[j];

            if (objectA->CanCollideWith(objectB->GetObjectType()) && objectA->DoesCollideWith(objectB))
            {
                if (objectA->GetObjectTypeName() != objectB->GetObjectTypeName())
                {
                    // log
                }
                objectA->ResolveCollision(objectB);
            }

            if (objectB->CanCollideWith(objectA->GetObjectType()) && objectB->DoesCollideWith(objectA))
            {
                if (objectB->GetObjectTypeName() != objectA->GetObjectTypeName())
                {
                    // log
                }
                objectB->ResolveCollision(objectA);
            }
        }
    }
}

const std::list<std::unique_ptr<GameObject>>& GameObjectManager::Objects() const
{
    return gameObjects;
}






