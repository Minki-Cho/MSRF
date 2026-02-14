#include "ObjectPool.h"
#include <iostream>

struct TestObj {
    int x;
    TestObj(int v) : x(v) {}
    ~TestObj() { /* cleanup */ }
};

void PoolSmokeTest()
{
    ObjectPool<TestObj, 4> pool;

    auto* a = pool.Allocate(1);
    auto* b = pool.Allocate(2);
    std::cout << a->x << ", " << b->x << "\n";

    pool.Free(a);
    pool.Free(b);

    std::cout << "InUse=" << pool.InUse() << " Available=" << pool.Available() << "\n";
}
