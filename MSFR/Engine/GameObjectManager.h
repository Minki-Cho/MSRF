#pragma once
#include <atomic>
#include <list>
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
    const std::list<std::unique_ptr<GameObject>>& Objects() const;

private:
    void FlushPendingAdds();

private:
    std::list<std::unique_ptr<GameObject>> gameObjects;

    std::mutex pendingMutex;
    std::vector<std::unique_ptr<GameObject>> pendingAdd;

    std::atomic<bool> isUpdating{ false };
};
