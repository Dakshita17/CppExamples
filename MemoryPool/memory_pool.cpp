#include <algorithm>
#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>
#include <iostream>

struct Marketplace {
    int price;
};

struct FreeNode {
    FreeNode* next;
};

class MemoryPool {
    std::size_t slots_;
    void* storage_;
    FreeNode* freeNode_;

public:
    explicit MemoryPool(std::size_t slots);
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;

    Marketplace* allocate();
    void deallocate(Marketplace* ptr);
    ~MemoryPool();
};

MemoryPool::MemoryPool(std::size_t slots)
    : slots_(slots), storage_(nullptr), freeNode_(nullptr) {
    if (slots_ == 0) {
        return;
    }

    const std::size_t requiredSize = std::max(sizeof(FreeNode), sizeof(Marketplace));
    const std::size_t alignment = std::max(alignof(FreeNode), alignof(Marketplace));
    const std::size_t slotSize =
        (requiredSize / alignment + (requiredSize % alignment != 0)) * alignment;

    if (slots_ > std::numeric_limits<std::size_t>::max() / slotSize) {
        throw std::length_error("Memory pool size overflow");
    }

    storage_ = ::operator new(slots_ * slotSize);
    auto* base = static_cast<char*>(storage_);

    freeNode_ = ::new (storage_) FreeNode{nullptr};
    FreeNode* curr = freeNode_;
    for (std::size_t i = 1; i < slots_; ++i) {
        void* address = base + i * slotSize;
        auto* node = ::new (address) FreeNode{nullptr};
        curr->next = node;
        curr = node;
    }
}

Marketplace* MemoryPool::allocate() {
    if (freeNode_ == nullptr) {
        return nullptr;
    }

    FreeNode* start = freeNode_;
    freeNode_ = start->next;
    return ::new (static_cast<void*>(start)) Marketplace{};
}

void MemoryPool::deallocate(Marketplace* ptr) {
    if (ptr == nullptr) {
        return;
    }

    // Contract: ptr must be a currently allocated object from this pool.
    ptr->~Marketplace();
    auto* node = ::new (static_cast<void*>(ptr)) FreeNode{freeNode_};
    freeNode_ = node;
}

MemoryPool::~MemoryPool() {
    // Marketplace is trivially destructible in this example.
    ::operator delete(storage_);
}

int main() {
    MemoryPool pool(2);
    Marketplace* first = pool.allocate();
    Marketplace* second = pool.allocate();
    Marketplace* third = pool.allocate();

    if (!first || !second || third != nullptr) {
        return 1;
    }

    first->price = 100;
    second->price = 200;
    pool.deallocate(first);
    Marketplace* reused = pool.allocate();

    const bool sameAddress = (reused == first);
    std::cout << "Pool reused the first slot: " << std::boolalpha << sameAddress << '\n';
    pool.deallocate(reused);
    pool.deallocate(second);
    return sameAddress ? 0 : 1;
}
