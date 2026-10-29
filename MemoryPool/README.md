# Fixed-Size Memory Pool — C++ Interview Notes

A small, fixed-capacity memory pool for `Marketplace` objects. See
[`memory_pool.cpp`](memory_pool.cpp).

## Why a memory pool?

Calling `new`/`delete` for every short-lived object can add allocation
overhead and latency variability. A pool allocates **one contiguous
backing block** up front and reuses its slots. This example is
single-threaded, fixed-capacity, and intended for learning—not a
production allocator.

## Slot layout, alignment, and padding

A free slot holds a `FreeNode { FreeNode* next; }`; an occupied slot
holds a `Marketplace { int price; }`. They occupy the **same bytes at
different times**, not simultaneously.

On a typical 64-bit platform:

| Type | `sizeof` | `alignof` |
|---|---:|---:|
| `Marketplace` | 4 | 4 |
| `FreeNode` | 8 | 8 |

These values are platform-dependent; the program uses `sizeof` and
`alignof` rather than hard-coding them.

1. `requiredSize = max(sizeof(FreeNode), sizeof(Marketplace))`
2. `alignment = max(alignof(FreeNode), alignof(Marketplace))`
3. `slotSize = ceil(requiredSize / alignment) * alignment`
4. `totalBytes = slots * slotSize`

Integer arithmetic avoids floating point:

```cpp
slotSize = (requiredSize / alignment +
            (requiredSize % alignment != 0)) * alignment;
```

**Padding example:** if a slot needs 18 bytes with alignment 8, round up
to **24 bytes**. Slots start at `base + i * slotSize`, so every slot
starts at a properly aligned address. `char*` pointer arithmetic moves
in **bytes**, unlike `T*` pointer arithmetic.

`::operator new(totalBytes)` supplies suitably aligned storage for
ordinary-alignment types used here; over-aligned types need an aligned
allocation strategy.

## Free list and object lifetimes

Initially, for three slots:

```text
freeNode -> [slot A: next=B] -> [slot B: next=C] -> [slot C: next=null]
```

- **Constructor:** allocate raw storage with `::operator new`, then
  placement-construct a `FreeNode` in each slot and link them. Raw
  allocation does **not** construct objects.
- **allocate():** return `nullptr` if the list is empty; otherwise
  save its head, advance `freeNode` to `head->next`, and
  placement-construct `Marketplace` at the saved address. Save the
  next pointer before reusing the slot.
- **deallocate(ptr):** end the `Marketplace` lifetime with
  `ptr->~Marketplace()`, placement-construct `FreeNode{freeNode}` in
  the same slot, and set `freeNode = node`. Inserting at the head
  takes **O(1)** time.
- **Pool destructor:** release the backing block using
  `::operator delete(storage_)`, matching `::operator new`.

**Placement new:** `::new (address) Type{}` constructs an object at
existing storage; it does not allocate another block. Include `<new>`.

## Complexity and behavior

| Operation | Time | Notes |
|---|---:|---|
| Construct pool | O(N) | One backing allocation, link N slots |
| `allocate()` | O(1) | Returns `nullptr` when full |
| `deallocate()` | O(1) | Pushes slot onto free-list head |
| Destroy pool | O(1) | One backing deallocation for this trivial type |

With two slots: allocate A, allocate B, a third allocation returns
`nullptr`; deallocate A; the next allocation returns **the same address A**.

## Fixes and boundaries to remember

- **Zero capacity:** do not construct slot 0 if `slots == 0`; the pool
  stays empty. This avoids unsigned-underflow and invalid access.
- **Free-list termination:** each newly constructed node starts with
  `FreeNode{nullptr}`; do not reset `freeNode->next` after linking,
  because that would disconnect the rest of the list.
- **Ownership:** copy and move operations are deleted. A default copy
  would make two pools own the same block and double-free it.
- **Overflow:** check `slots * slotSize` before allocating.
- **Null deallocation:** this implementation treats
  `deallocate(nullptr)` as a no-op.
- **Invalid pointers/double deallocation:** not detected. Passing a
  pointer from another pool, or deallocating twice, violates the API
  contract and can cause undefined behavior or free-list cycles.
- **Object destruction:** `Marketplace` is trivially destructible. A
  general pool of non-trivial objects must track live slots and destroy
  them before releasing storage, or require clients to return every
  object.
- **Thread safety:** none. Concurrent access requires synchronization
  or a different design.
- **Capacity:** fixed. The pool does not grow when exhausted.

## Quick interview explanation

> I preallocate one aligned block and divide it into fixed-size slots.
> Each free slot stores the next pointer for an intrusive singly linked
> free list. Allocation pops the head and placement-constructs the
> object in that slot. Deallocation destroys the object and pushes the
> slot back onto the head. Both operations are O(1) and need no
> per-object heap allocation. I round the slot size up to the maximum
> required alignment so each slot can safely hold either the object or
> the free-list node.

## Build and run

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined \
  memory_pool.cpp -o memory_pool
./memory_pool
```

Expected output: `Pool reused the first slot: true`.
