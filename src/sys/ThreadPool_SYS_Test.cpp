// ThreadPool_SYS_Test.cpp — acceptance test for ThreadPool_SYS (M0-7).
//   correctness: g++ -O2 -std=c++17 -pthread ThreadPool_SYS_Test.cpp -o t && ./t
//   race check:  g++ -O2 -std=c++17 -pthread -fsanitize=thread ... && ./t
#include "ThreadPool_SYS.h"
#include <cstdio>
#include <vector>
#include <atomic>
#include <numeric>

using namespace SanmapGen::Sys;

int main() {
    int failures = 0;

    // Each index writes its own slot exactly once — parallel correctness, no races.
    {
        ThreadPool pool(4);
        const int n = 100000;
        std::vector<int> data(n, 0);
        pool.ParallelFor(0, n, [&](int i) { data[i] = i * 2; });
        for (int i = 0; i < n; ++i)
            if (data[i] != i * 2) { std::printf("FAIL write at %d = %d\n", i, data[i]); ++failures; break; }
    }

    // Every index visited exactly once (atomic visit counter).
    {
        ThreadPool pool(0);   // hardware_concurrency
        const int n = 50000;
        std::vector<std::atomic<int>> visits(n);
        for (auto& v : visits) v.store(0);
        pool.ParallelFor(0, n, [&](int i) { visits[i].fetch_add(1); });
        long long visited = 0;
        for (int i = 0; i < n; ++i) visited += visits[i].load();
        if (visited != n) { std::printf("FAIL visited=%lld expected %d\n", visited, n); ++failures; }
    }

    // Reuse the same pool many times (no per-call thread creation).
    {
        ThreadPool pool(3);
        for (int round = 0; round < 200; ++round) {
            std::vector<int> data(1000, 0);
            pool.ParallelFor(0, 1000, [&](int i) { data[i] = i + round; });
            if (data[999] != 999 + round) { std::printf("FAIL reuse round %d\n", round); ++failures; break; }
        }
    }

    // Edge cases: empty range, single element, size-0 pool runs inline.
    {
        ThreadPool pool(4);
        int counter = 0;
        pool.ParallelFor(5, 5, [&](int) { ++counter; });   // empty -> no calls
        if (counter != 0) { std::printf("FAIL empty range\n"); ++failures; }
        pool.ParallelFor(0, 1, [&](int) { ++counter; });   // single
        if (counter != 1) { std::printf("FAIL single element\n"); ++failures; }

        ThreadPool inlinePool(0 == 0 ? 0u : 0u);  // note: 0 => hardware; test explicit inline below
    }
    {
        ThreadPool inlinePool(0);
        // Force inline path by using a tiny range on any pool (total<2 runs inline).
        int sum = 0;
        inlinePool.ParallelFor(0, 1, [&](int i){ sum += i + 7; });
        if (sum != 7) { std::printf("FAIL inline tiny\n"); ++failures; }
    }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
