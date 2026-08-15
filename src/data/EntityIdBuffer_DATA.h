// EntityIdBuffer_DATA.h — per-pixel entity-ID buffer (row-major) for O(1) picking.
// Layer: DATA. The CPU-side readback target the preview composite writes an entity ID
// into per pixel; a click resolves one cell instead of testing 100k items.
// Plain data + accessors; no behavior, no GPU handles (ARCH §3.2) — the GPU-side copy
// is owned by SYS. Written by the composite (M4-3), read by picking (M4-4).
#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

namespace SanmapGen {
namespace Data {

class EntityIdBuffer {
public:
    // A cell holding this value covers empty space — no entity was rendered there.
    static constexpr std::uint32_t emptySentinel = 0xFFFFFFFFu;

    EntityIdBuffer() : bufferWidth(0), bufferHeight(0) {}
    EntityIdBuffer(int width, int height) { Resize(width, height); }

    // Reshapes to the preview resolution; every cell starts empty.
    void Resize(int width, int height) {
        bufferWidth = width < 0 ? 0 : width;
        bufferHeight = height < 0 ? 0 : height;
        entityIds.assign(static_cast<std::size_t>(bufferWidth) * bufferHeight, emptySentinel);
    }

    int Width() const { return bufferWidth; }
    int Height() const { return bufferHeight; }
    std::size_t CellCount() const { return entityIds.size(); }
    bool IsEmpty() const { return entityIds.empty(); }

    std::uint32_t Get(int x, int y) const { return entityIds[CellIndex(x, y)]; }
    void Set(int x, int y, std::uint32_t entityId) { entityIds[CellIndex(x, y)] = entityId; }

    std::uint32_t* Data() { return entityIds.data(); }
    const std::uint32_t* Data() const { return entityIds.data(); }

    // Marks every cell empty without reallocating — the composite's first pass.
    void Clear() {
        for (std::uint32_t& cell : entityIds) cell = emptySentinel;
    }

private:
    std::size_t CellIndex(int x, int y) const {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(bufferWidth)
             + static_cast<std::size_t>(x);
    }

    int bufferWidth;
    int bufferHeight;
    std::vector<std::uint32_t> entityIds;
};

} // namespace Data
} // namespace SanmapGen
