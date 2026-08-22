// RuleBucketIndex_DATA.h — a computed index over one Data::PlacementInstances collection's
// ruleIndex column: which SoA indices belong to rule N, in O(1) + O(bucket size), without ever
// physically resorting the SoA (ARCH_14_09_RenderingPerformance.md §14.9's "Layer-id column" ruling). Flat CSR buckets of
// std::int32_t indices, mechanically identical build shape to Data::SpatialGrid (count -> prefix
// sum -> scatter) — see that file's own header for why this shape, not vector<vector<>>.
// Single writer (ARCH_03_ModuleBoundaries.md §3.4.1): Pipeline::GenerationAssembler, right after Placement.
#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>

namespace SanmapGen {
namespace Data {

class RuleBucketIndex {
public:
    int BucketCount() const { return bucketCount; }
    std::int32_t EntryCount() const { return static_cast<std::int32_t>(instanceIndex.size()); }
    bool IsEmpty() const { return instanceIndex.empty(); }

    // [begin, end) into InstanceIndexAt for one rule; out-of-range buckets and every bucket of an
    // empty/unbuilt index answer with an empty range (mirrors SpatialGrid::BucketBegin/End).
    std::int32_t BucketBegin(int bucket) const {
        return IsValidBucket(bucket) ? bucketStart[static_cast<std::size_t>(bucket)] : 0;
    }
    std::int32_t BucketEnd(int bucket) const {
        return IsValidBucket(bucket) ? bucketStart[static_cast<std::size_t>(bucket) + 1] : 0;
    }
    // The entry at a position inside a bucket range: an index into the Data::PlacementInstances
    // collection this index was built over.
    std::int32_t InstanceIndexAt(std::int32_t position) const {
        return instanceIndex[static_cast<std::size_t>(position)];
    }
    const std::int32_t* InstanceIndexData() const { return instanceIndex.data(); }

    // `key` is one collection's ruleIndex column (e.g. results.props.ruleIndex.data()); `count`
    // its length; `bucketTotal` the caller's OWN rule-array size (e.g. recipe.propRules.size()) —
    // NOT re-derived from max(key), so a trailing rule with zero accepted instances still gets an
    // addressable empty bucket. A key outside [0, bucketTotal) is DROPPED, not clamped: unlike
    // SpatialGrid's world-position clamp (any spatial answer is "close enough"), silently
    // reattributing an instance to the wrong rule's bucket would corrupt sub-layer membership
    // invisibly — dropping is the safer defensive default for an identity key. In practice this
    // never triggers: PROC always assigns ruleIndex within range (Placement_Rules_PROC.cpp).
    // Replaces the previous contents; it never accumulates.
    void Build(const int* key, std::int32_t count, int bucketTotal) {
        bucketCount = bucketTotal > 0 ? bucketTotal : 0;
        bucketStart.assign(static_cast<std::size_t>(bucketCount) + 1, 0);
        instanceIndex.clear();
        if (count <= 0 || key == nullptr || bucketCount <= 0) return;
        for (std::int32_t entry = 0; entry < count; ++entry)
            if (IsValidBucket(key[entry])) ++bucketStart[static_cast<std::size_t>(key[entry]) + 1];
        for (int bucket = 0; bucket < bucketCount; ++bucket)
            bucketStart[static_cast<std::size_t>(bucket) + 1] += bucketStart[static_cast<std::size_t>(bucket)];
        instanceIndex.resize(static_cast<std::size_t>(bucketStart.back()));
        std::vector<std::int32_t> writeCursor(bucketStart.begin(), bucketStart.end() - 1);
        for (std::int32_t entry = 0; entry < count; ++entry) {
            if (!IsValidBucket(key[entry])) continue;
            const int bucket = key[entry];
            instanceIndex[static_cast<std::size_t>(writeCursor[static_cast<std::size_t>(bucket)]++)] = entry;
        }
    }

    void Clear() {   // an empty index is valid and answers every query with an empty range
        bucketStart.assign(static_cast<std::size_t>(bucketCount) + 1, 0);
        instanceIndex.clear();
    }

private:
    bool IsValidBucket(int bucket) const { return bucket >= 0 && bucket < bucketCount; }
    std::vector<std::int32_t> bucketStart;      // size bucketCount + 1, monotonic
    std::vector<std::int32_t> instanceIndex;    // size = accepted entries
    int bucketCount = 0;
};

} // namespace Data
} // namespace SanmapGen
