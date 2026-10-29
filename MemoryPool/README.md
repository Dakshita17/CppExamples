# Memory Pool

I implemented a simple memory pool for `Marketplace` objects.

The idea is to allocate one large chunk of memory in the constructor and divide it into slots. Each free slot stores a pointer to the next free slot using `FreeNode`.

## How it works

- In the constructor, I calculate the slot size using the size and alignment of `Marketplace` and `FreeNode`.
- I use placement new to create a `FreeNode` in each slot and link all the nodes together.
- In `allocate()`, I take the first node from the free list and create a `Marketplace` object in that memory.
- In `deallocate()`, I destroy the `Marketplace` object and put that slot back at the beginning of the free list.
- If there are no free slots, `allocate()` returns `nullptr`.

Both `allocate()` and `deallocate()` are O(1), because I only update the head of the linked list.

## A note on alignment

The slot needs to be large enough for both `Marketplace` and `FreeNode`, so I take the maximum of their sizes and alignments.

I round the required size up to the next multiple of the alignment:

```cpp
size_t gap = required_size / alignment +
             (required_size % alignment ? 1 : 0);
size_t slot_size = gap * alignment;
```

For example, if the required size is 18 bytes and alignment is 8, the slot size becomes 24 bytes. This padding makes sure every slot starts at a correctly aligned address.

## Placement new and object lifetime

Normally, when I write:

```cpp
Marketplace* ptr = new Marketplace();
```

C++ allocates memory and then constructs the `Marketplace` object in that memory.

In this memory pool, I have already allocated the memory for all the slots in the constructor. I only need to construct an object in a slot when it is used. This is what placement new does.

The general syntax is:

```cpp
Type* ptr = ::new (memory_address) Type(arguments);
```

For example:

```cpp
auto node = ::new (address) FreeNode{nullptr};
auto marketplace = ::new (start) Marketplace();
```

Placement new constructs the object at the given address. It does not allocate another block of memory.

There is an important difference between destroying an object and releasing the memory:

```cpp
ptr->~Marketplace();
```

This calls the destructor and ends the object's lifetime, but it does not release the memory. The slot is still part of the large memory block owned by the pool, so I can reuse it.

In `deallocate()`, after destroying the `Marketplace`, I use placement new again to construct a `FreeNode` in the same slot:

```cpp
mptr->~Marketplace();
auto node = ::new (mptr) FreeNode{static_cast<FreeNode*>(freeNode)};
freeNode = node;
```

The `FreeNode` takes the place of the `Marketplace` object and is added back to the free list.

## How memory is released

The pool allocates its large block using:

```cpp
storage = ::operator new(size);
```

And releases that block in the destructor using:

```cpp
::operator delete(storage);
```

These are a matching allocation and deallocation pair.

I should not use `delete mptr` for an object created in one of the pool's slots. Normal `delete` is for an object created with a matching normal `new` expression. Here, the pool owns the large memory block, not each individual slot. I call the object's destructor and then reuse the slot instead.

## Why I deleted the copy operations

I deleted the copy constructor and copy assignment operator:

```cpp
MemoryPool(const MemoryPool&) = delete;
MemoryPool& operator=(const MemoryPool&) = delete;
```

The pool owns memory through the `storage` pointer. If I allowed the default copy, both objects would end up with the same pointer.

When both pools are destroyed, both destructors would try to release the same memory. This can cause a double free.

Deleting these operations prevents that shallow copy. A proper copy would need to allocate a separate block and rebuild the pool's state, which I don't need for this example.

## Things to remember

- Placement new constructs an object in memory that is already allocated. It does not allocate another block.
- Calling `ptr->~Type()` ends the object's lifetime, but does not release the memory.
- Normal `delete ptr` destroys an object and releases its allocation. I should not use it for an object inside this pool.
- I need to read `start->next` before constructing `Marketplace` in that slot, because the `FreeNode` object is replaced.
- The loop uses `i < size`, so the pool also handles zero slots without unsigned underflow.
- The pool is fixed-size. Once all slots are allocated, it returns `nullptr`.
- This is a simple single-threaded implementation. The pointer passed to `deallocate()` should be an allocated pointer from this pool, and should only be deallocated once.
- `Marketplace` currently only has an `int`, so its destructor has no extra cleanup to do. If I use this pool for objects that own resources, I also need to make sure their destructors are called before releasing the whole memory block.
