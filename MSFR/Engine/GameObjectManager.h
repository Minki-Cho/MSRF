#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "Component.h"
#include "GameObject.h"
#include "mat3.h"

class GameObjectManager : public Component
{
public:
    ~GameObjectManager() override;

    void Add(std::unique_ptr<GameObject> obj);
    void Add(GameObject* obj);

    void Update(double dt) override;
    void DrawAll(mat3<float>& cameraMatrix);
    void CollideTest();
    const std::vector<std::unique_ptr<GameObject>>& Objects() const;

private:
    struct Bounds2D
    {
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
    };

    bool TryGetBounds(GameObject* object, Bounds2D& outBounds) const;
    std::uint64_t MakePairKey(std::uint32_t a, std::uint32_t b) const;
    std::int64_t MakeCellKey(int x, int y) const;
    void ReclaimDestroyed();
    void StoreObject(std::unique_ptr<GameObject> obj);
    void FlushPendingAdds();

private:
    std::vector<std::unique_ptr<GameObject>> gameObjects;
    std::vector<std::uint32_t> freeIndices;

    std::mutex pendingMutex;
    std::vector<std::unique_ptr<GameObject>> pendingAdd;

    std::atomic<bool> isUpdating{ false };

    static constexpr float BroadphaseCellSize = 220.0f;
};
