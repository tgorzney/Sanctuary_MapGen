// ArenaAllocator_SYS.h — linear "bump" allocator for scratch memory.
// Layer: SYS. Reserves one contiguous block up front and hands out aligned slices by
// advancing an offset — O(1) allocation, no per-object free. Reset() reclaims the whole
// arena at once (per-frame / per-pass / thread-local scratch; an optimization pillar).
// Out-of-space returns nullptr (never grows, never throws). Not thread-safe by design:
// give each worker its own arena.
#pragma once
#include <cstddef>
#include <cstdint>

namespace SanmapGen {
namespace Sys {

class ArenaAllocator {
public:
    explicit ArenaAllocator(std::size_t capacityBytes)
        : baseAllocation(new unsigned char[capacityBytes + alignmentReserve]),
          capacity(capacityBytes), usedOffset(0) {
        std::size_t rawAddress = reinterpret_cast<std::size_t>(baseAllocation);
        std::size_t alignedAddress = (rawAddress + (alignmentReserve - 1)) & ~std::size_t(alignmentReserve - 1);
        alignedBase = reinterpret_cast<unsigned char*>(alignedAddress);
    }
    ~ArenaAllocator() { delete[] baseAllocation; }

    ArenaAllocator(const ArenaAllocator&) = delete;
    ArenaAllocator& operator=(const ArenaAllocator&) = delete;
    ArenaAllocator(ArenaAllocator&& other) noexcept
        : baseAllocation(other.baseAllocation), alignedBase(other.alignedBase),
          capacity(other.capacity), usedOffset(other.usedOffset) {
        other.baseAllocation = nullptr; other.alignedBase = nullptr; other.capacity = 0; other.usedOffset = 0;
    }
    ArenaAllocator& operator=(ArenaAllocator&& other) noexcept {
        if (this != &other) {
            delete[] baseAllocation;
            baseAllocation = other.baseAllocation; alignedBase = other.alignedBase;
            capacity = other.capacity; usedOffset = other.usedOffset;
            other.baseAllocation = nullptr; other.alignedBase = nullptr; other.capacity = 0; other.usedOffset = 0;
        }
        return *this;
    }

    // Returns an aligned block of sizeBytes, or nullptr if the arena is full.
    // alignmentBytes must be a power of two and <= alignmentReserve.
    void* Allocate(std::size_t sizeBytes, std::size_t alignmentBytes = alignof(std::max_align_t)) {
        std::size_t base = reinterpret_cast<std::size_t>(alignedBase);
        std::size_t candidate = (base + usedOffset + (alignmentBytes - 1)) & ~(alignmentBytes - 1);
        std::size_t nextOffset = (candidate - base) + sizeBytes;
        if (nextOffset > capacity) return nullptr;
        usedOffset = nextOffset;
        return reinterpret_cast<void*>(candidate);
    }

    template <typename ElementType>
    ElementType* AllocateArray(std::size_t count, std::size_t alignmentBytes = alignof(ElementType)) {
        return static_cast<ElementType*>(Allocate(count * sizeof(ElementType), alignmentBytes));
    }

    void Reset() { usedOffset = 0; }
    std::size_t BytesUsed() const { return usedOffset; }
    std::size_t CapacityBytes() const { return capacity; }

private:
    static constexpr std::size_t alignmentReserve = 64;  // supports up to 64-byte alignment
    unsigned char* baseAllocation;
    unsigned char* alignedBase = nullptr;
    std::size_t capacity;
    std::size_t usedOffset;
};

} // namespace Sys
} // namespace SanmapGen
