// Generation_PIPELINE.h — the dirty-hash stage conductor.
// Layer: PIPELINE. Holds the ordered generation stages and re-runs only the ones whose
// inputs changed. Each stage contributes a hash of its own params; that mixes forward
// with the upstream hash, so changing an early stage's params dirties it and everything
// downstream, while an unchanged prefix is skipped (reuses cached output). Stages are
// injected (a stage's Run closure calls PROC via SYS dispatch) so this file has no
// dependency on any concrete stage — it is pure ordering + dirty tracking.
#pragma once
#include <vector>
#include <string>
#include <functional>
#include <cstddef>

namespace SanmapGen {
namespace Pipeline {

inline std::size_t HashCombine(std::size_t seed, std::size_t value) {
    return seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
}

class GenerationPipeline {
public:
    // computeParamHash: hash of this stage's own settings (changes => stage dirty).
    // run: perform the stage (dispatches CPU/GPU internally).
    void AddStage(const std::string& name,
                  std::function<std::size_t()> computeParamHash,
                  std::function<void()> run) {
        stages.push_back(Stage{ name, std::move(computeParamHash), std::move(run) });
        cachedCombinedHash.push_back(hashUnset);
    }

    std::size_t StageCount() const { return stages.size(); }

    // Runs the pipeline in order; a stage runs only when its combined (upstream + own
    // params) hash differs from the cached value. Returns the names that actually ran.
    std::vector<std::string> Run() {
        std::vector<std::string> ran;
        std::size_t upstream = baseHash;
        for (std::size_t index = 0; index < stages.size(); ++index) {
            std::size_t combined = HashCombine(upstream, stages[index].computeParamHash());
            if (combined != cachedCombinedHash[index]) {
                stages[index].run();
                cachedCombinedHash[index] = combined;
                ran.push_back(stages[index].name);
            }
            upstream = combined;
        }
        return ran;
    }

    // Force the whole pipeline to re-run on the next Run() (e.g. after a resize).
    void InvalidateAll() {
        for (std::size_t& cached : cachedCombinedHash) cached = hashUnset;
    }

private:
    struct Stage {
        std::string name;
        std::function<std::size_t()> computeParamHash;
        std::function<void()> run;
    };
    static constexpr std::size_t baseHash = 1469598103934665603ull;  // FNV offset basis
    static constexpr std::size_t hashUnset = ~std::size_t(0);

    std::vector<Stage> stages;
    std::vector<std::size_t> cachedCombinedHash;
};

} // namespace Pipeline
} // namespace SanmapGen
