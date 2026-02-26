#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include <list>

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
    void FlushPendingAdds(); // main-thread only

private:
    std::list<GameObject*> gameObjects;

    std::mutex pendingMutex;
    std::vector<GameObject*> pendingAdd;

    std::atomic<bool> isUpdating{ false };
};