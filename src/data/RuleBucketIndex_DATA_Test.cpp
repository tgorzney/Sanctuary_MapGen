// RuleBucketIndex_DATA_Test.cpp — acceptance test for STEP50 (RuleBucketIndex_DATA).
//   g++ -O2 -std=c++17 -fsanitize=address,undefined RuleBucketIndex_DATA_Test.cpp -o t && ./t
#include "RuleBucketIndex_DATA.h"
#include <cstdio>
#include <cstdint>
#include <vector>

using namespace SanmapGen::Data;

static int failures = 0;
static void check(bool ok, const char* label) { if (!ok) { std::printf("FAIL: %s\n", label); ++failures; } }

int main() {
    // ---- An unbuilt/empty index (bucketTotal == 0, count == 0) answers every query empty.
    RuleBucketIndex index;
    check(index.IsEmpty() && index.EntryCount() == 0, "starts empty");
    check(index.BucketCount() == 0, "starts with zero buckets");
    for (int bucket = -1; bucket <= 4; ++bucket)
        if (index.BucketBegin(bucket) != 0 || index.BucketEnd(bucket) != 0)
            { check(false, "empty index answers every bucket empty"); break; }

    index.Build(nullptr, 0, 0);
    check(index.IsEmpty() && index.BucketCount() == 0, "build with bucketTotal==0 and count==0 stays empty");

    // ---- A known deterministic set of ruleIndex values across several buckets.
    // 12 instances across 4 buckets (rule 0..3), uneven distribution.
    const std::vector<int> key = { 0, 2, 1, 0, 3, 2, 0, 1, 2, 2, 3, 0 };
    const std::int32_t count = static_cast<std::int32_t>(key.size());
    const int bucketTotal = 4;

    index.Build(key.data(), count, bucketTotal);
    check(index.BucketCount() == bucketTotal, "bucket count matches caller's rule-array size");
    check(index.EntryCount() == count, "every in-range entry indexed");
    check(!index.IsEmpty(), "no longer empty after build");

    std::int32_t previousEnd = 0;
    std::vector<int> timesSeen(static_cast<std::size_t>(count), 0);
    for (int bucket = 0; bucket < index.BucketCount(); ++bucket) {
        const std::int32_t begin = index.BucketBegin(bucket), end = index.BucketEnd(bucket);
        if (begin != previousEnd || end < begin) { check(false, "prefix sum monotonic"); break; }
        previousEnd = end;
        for (std::int32_t position = begin; position < end; ++position) {
            const std::int32_t entry = index.InstanceIndexAt(position);
            if (key[static_cast<std::size_t>(entry)] != bucket)
                { check(false, "instance resolves back to its own bucket via the key array"); break; }
            ++timesSeen[static_cast<std::size_t>(entry)];
        }
    }
    check(previousEnd == count, "bucketStart[bucketCount] == count");
    for (std::int32_t entry = 0; entry < count; ++entry)
        if (timesSeen[static_cast<std::size_t>(entry)] != 1) { check(false, "each instance appears exactly once"); break; }

    // ---- A trailing rule with zero accepted instances still gets an addressable, empty, in-range
    // bucket — bucketTotal is larger than max(key) + 1.
    const std::vector<int> keyNoTrailing = { 0, 1, 0, 1 };
    RuleBucketIndex trailing;
    trailing.Build(keyNoTrailing.data(), static_cast<std::int32_t>(keyNoTrailing.size()), 5);
    check(trailing.BucketCount() == 5, "bucket count sized from caller's rule count, not max(key)");
    check(trailing.BucketBegin(4) == trailing.BucketEnd(4), "trailing zero-instance bucket is empty");
    check(trailing.BucketBegin(2) == trailing.BucketEnd(2) && trailing.BucketBegin(3) == trailing.BucketEnd(3),
          "interior zero-instance buckets are also empty, addressable ranges");
    check(trailing.EntryCount() == static_cast<std::int32_t>(keyNoTrailing.size()), "no entries dropped");

    // ---- An out-of-range key (>= bucketTotal or negative) is dropped, not clamped, not crashing.
    const std::vector<int> keyOutOfRange = { 0, 1, -1, 2, 100, 0, 1 };
    RuleBucketIndex dropped;
    dropped.Build(keyOutOfRange.data(), static_cast<std::int32_t>(keyOutOfRange.size()), 2);
    check(dropped.BucketCount() == 2, "bucket count still the caller's bucketTotal");
    check(dropped.EntryCount() == 4, "only in-range entries (0,1,0,1) survive");
    std::int32_t droppedPreviousEnd = 0;
    for (int bucket = 0; bucket < dropped.BucketCount(); ++bucket) {
        const std::int32_t begin = dropped.BucketBegin(bucket), end = dropped.BucketEnd(bucket);
        for (std::int32_t position = begin; position < end; ++position) {
            const std::int32_t entry = dropped.InstanceIndexAt(position);
            if (keyOutOfRange[static_cast<std::size_t>(entry)] != bucket)
                { check(false, "dropped-key entries still resolve correctly"); break; }
        }
        droppedPreviousEnd = end;
    }
    check(droppedPreviousEnd == 4, "dropped entries excluded from EntryCount/bucket ranges");

    // ---- Rebuild replaces, does not accumulate.
    index.Build(key.data(), count, bucketTotal);
    index.Build(key.data(), count, bucketTotal);
    check(index.EntryCount() == count, "rebuild replaces, does not accumulate");

    // ---- count <= 0, a nullptr key column, and Clear() are all safe.
    RuleBucketIndex safety;
    safety.Build(key.data(), 0, bucketTotal);
    check(safety.IsEmpty() && safety.BucketCount() == bucketTotal, "count == 0 build safe, buckets still sized");
    safety.Build(nullptr, count, bucketTotal);
    check(safety.IsEmpty(), "null key column safe");
    safety.Build(key.data(), -1, bucketTotal);
    check(safety.IsEmpty(), "negative count safe");
    safety.Build(key.data(), count, bucketTotal);
    check(!safety.IsEmpty(), "sanity: a valid build after the safety checks still works");
    safety.Clear();
    check(safety.IsEmpty() && safety.BucketBegin(1) == safety.BucketEnd(1), "clear empties without dropping bucket addressability");

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
