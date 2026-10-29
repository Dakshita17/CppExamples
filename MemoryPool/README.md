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

## Things to remember

- Placement new constructs an object in memory that is already allocated.
- I need to read `start->next` before constructing `Marketplace` in that slot, because the `FreeNode` object is replaced.
- The loop uses `i < size`, so the pool also handles zero slots without unsigned underflow.
- The pool is fixed-size. Once all slots are allocated, it returns `nullptr`.
- This is a simple single-threaded implementation. The pointer passed to `deallocate()` should be an allocated pointer from this pool, and should only be deallocated once.
