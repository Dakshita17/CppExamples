#include<iostream>
#include <algorithm>
#include <cstddef>
#include <new>

using namespace std;

struct Marketplace {
    int price;
};

struct FreeNode {
    FreeNode* next;
};

class MemoryPool {
    int size;
    void* storage;
    void* freeNode;

public:
    MemoryPool(int slots);
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    Marketplace* allocate();
    void deallocate(Marketplace* mptr);
    ~MemoryPool();
};

MemoryPool::MemoryPool(int slots) : size(slots), storage(nullptr), freeNode(nullptr)
{
    if (size <= 0)
        return;

    size_t required_size = max(sizeof(FreeNode), sizeof(Marketplace));
    size_t alignment = max(alignof(FreeNode), alignof(Marketplace));

    size_t gap = required_size / alignment +
                 (required_size % alignment ? 1 : 0);
    size_t slot_size = gap * alignment;

    storage = ::operator new(static_cast<size_t>(size) * slot_size);
    char* base = static_cast<char*>(storage);

    FreeNode* curr = nullptr;

    for (int i = 0; i < size; ++i) {
        void* address = base + static_cast<size_t>(i) * slot_size;
        auto node = ::new (address) FreeNode{nullptr};

        if (curr == nullptr)
            freeNode = node;
        else
            curr->next = node;

        curr = node;
    }
}

Marketplace* MemoryPool::allocate()
{
    if (freeNode == nullptr)
        return nullptr;

    auto start = static_cast<FreeNode*>(freeNode);
    freeNode = start->next;

    auto node = ::new (start) Marketplace();
    return node;
}

void MemoryPool::deallocate(Marketplace* mptr)
{
    if (mptr == nullptr)
        return;

    mptr->~Marketplace();

    auto node = ::new (mptr) FreeNode{static_cast<FreeNode*>(freeNode)};
    freeNode = node;
}

MemoryPool::~MemoryPool()
{
    ::operator delete(storage);
}
