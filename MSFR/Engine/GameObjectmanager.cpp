#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>

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

void GameObjectManager::StoreObject(std::unique_ptr<GameObject> obj)
{
    if (!obj)
        return;

    if (!freeIndices.empty())
    {
        const std::uint32_t index = freeIndices.back();
        freeIndices.pop_back();

        if (index < gameObjects.size())
        {
            gameObjects[index] = std::move(obj);
            return;
        }
    }

    gameObjects.push_back(std::move(obj));
}

void GameObjectManager::FlushPendingAdds()
{
    std::vector<std::unique_ptr<GameObject>> local;
    {
        std::lock_guard<std::mutex> lock(pendingMutex);
        local.swap(pendingAdd);
    }

    for (auto& obj : local)
        StoreObject(std::move(obj));
}

void GameObjectManager::ReclaimDestroyed()
{
    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(gameObjects.size()); ++i)
    {
        std::unique_ptr<GameObject>& obj = gameObjects[i];
        if (!obj || !obj->GetDestroyed())
            continue;

        obj.reset();
        freeIndices.push_back(i);
    }
}

void GameObjectManager::Update(double dt)
{
    FlushPendingAdds();

    const bool paused = Engine::GetInput().getPause();
    if (paused)
    {
        ReclaimDestroyed();
        FlushPendingAdds();
        return;
    }

    isUpdating.store(true, std::memory_order_release);

    std::vector<GameObject*> snapshot;
    snapshot.reserve(gameObjects.size());
    for (const auto& obj : gameObjects)
    {
        if (!obj || obj->GetDestroyed())
            continue;
        snapshot.push_back(obj.get());
    }

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

    ReclaimDestroyed();
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
    std::vector<Bounds2D> bounds;
    collidables.reserve(gameObjects.size());
    bounds.reserve(gameObjects.size());

    for (const auto& objectOwner : gameObjects)
    {
        GameObject* object = objectOwner.get();
        if (!object || object->GetDestroyed())
            continue;

        Bounds2D objectBounds{};
        if (!TryGetBounds(object, objectBounds))
            continue;

        collidables.push_back(object);
        bounds.push_back(objectBounds);
    }

    if (collidables.size() < 2)
        return;

    auto resolvePair = [](GameObject* objectA, GameObject* objectB)
    {
        if (!objectA || !objectB || objectA->GetDestroyed() || objectB->GetDestroyed())
            return;

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
    };

    // For small populations, direct pair iteration is usually faster than hashing.
    constexpr std::size_t kBroadphaseThreshold = 48;
    if (collidables.size() <= kBroadphaseThreshold)
    {
        const std::size_t n = collidables.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            for (std::size_t j = i + 1; j < n; ++j)
            {
                resolvePair(collidables[i], collidables[j]);
            }
        }
        return;
    }

    std::unordered_map<std::int64_t, std::vector<std::uint32_t>> cells;
    cells.reserve(collidables.size() * 2);

    const float invCellSize = 1.0f / BroadphaseCellSize;
    for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(collidables.size()); ++i)
    {
        const Bounds2D& b = bounds[i];

        const int minCellX = static_cast<int>(std::floor(b.minX * invCellSize));
        const int maxCellX = static_cast<int>(std::floor(b.maxX * invCellSize));
        const int minCellY = static_cast<int>(std::floor(b.minY * invCellSize));
        const int maxCellY = static_cast<int>(std::floor(b.maxY * invCellSize));

        for (int cy = minCellY; cy <= maxCellY; ++cy)
        {
            for (int cx = minCellX; cx <= maxCellX; ++cx)
            {
                const std::int64_t key = MakeCellKey(cx, cy);
                cells[key].push_back(i);
            }
        }
    }

    std::unordered_set<std::uint64_t> testedPairs;
    testedPairs.reserve(collidables.size() * 4);

    for (const auto& entry : cells)
    {
        const std::vector<std::uint32_t>& bucket = entry.second;
        const std::size_t n = bucket.size();
        if (n < 2)
            continue;

        for (std::size_t ai = 0; ai < n; ++ai)
        {
            const std::uint32_t i = bucket[ai];
            GameObject* objectA = collidables[i];
            if (!objectA || objectA->GetDestroyed())
                continue;

            for (std::size_t bi = ai + 1; bi < n; ++bi)
            {
                const std::uint32_t j = bucket[bi];
                const std::uint64_t pairKey = MakePairKey(i, j);
                if (!testedPairs.insert(pairKey).second)
                    continue;

                GameObject* objectB = collidables[j];
                resolvePair(objectA, objectB);
            }
        }
    }
}

bool GameObjectManager::TryGetBounds(GameObject* object, Bounds2D& outBounds) const
{
    if (!object)
        return false;

    Collision* collision = object->GetGOComponent<Collision>();
    if (!collision)
        return false;

    if (collision->GetCollideType() == Collision::CollideType::Rect_Collide)
    {
        RectCollision* rect = object->GetGOComponent<RectCollision>();
        if (!rect)
            return false;

        const rect3 world = rect->GetWorldCoorRect();
        outBounds.minX = world.Left();
        outBounds.maxX = world.Right();
        outBounds.minY = world.Bottom();
        outBounds.maxY = world.Top();
        return true;
    }

    if (collision->GetCollideType() == Collision::CollideType::Circle_Collide)
    {
        CircleCollision* circle = object->GetGOComponent<CircleCollision>();
        if (!circle)
            return false;

        const vec2 pos = object->GetPosition();
        const float r = static_cast<float>(circle->GetRadius());
        outBounds.minX = pos.x() - r;
        outBounds.maxX = pos.x() + r;
        outBounds.minY = pos.y() - r;
        outBounds.maxY = pos.y() + r;
        return true;
    }

    return false;
}

std::uint64_t GameObjectManager::MakePairKey(std::uint32_t a, std::uint32_t b) const
{
    if (a > b)
        std::swap(a, b);

    return (static_cast<std::uint64_t>(a) << 32) | static_cast<std::uint64_t>(b);
}

std::int64_t GameObjectManager::MakeCellKey(int x, int y) const
{
    const std::uint64_t ux = static_cast<std::uint32_t>(x);
    const std::uint64_t uy = static_cast<std::uint32_t>(y);
    const std::uint64_t packed = (ux << 32) | uy;
    return static_cast<std::int64_t>(packed);
}

const std::vector<std::unique_ptr<GameObject>>& GameObjectManager::Objects() const
{
    return gameObjects;
}






