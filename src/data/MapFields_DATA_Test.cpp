// MapFields_DATA_Test.cpp — acceptance test for M1-2 (MapFields + Layer + enums).
//   g++ -O2 -std=c++17 -I<src> -fsanitize=address,undefined MapFields_DATA_Test.cpp -o t && ./t
#include "MapFields_DATA.h"
#include "../params/Layer_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;

int main() {
    int failures = 0;

    Params::Layer layer;
    if (!layer.bEnabled || layer.octaves != 5 || layer.blendMode != Params::HeightBlendMode::Add)
        { std::printf("FAIL layer defaults\n"); ++failures; }
    if (Params::NoiseType::None == Params::NoiseType::Perlin) { std::printf("FAIL enum distinct\n"); ++failures; }

    Data::MapFields fields;
    if (fields.IsSized()) { std::printf("FAIL starts unsized\n"); ++failures; }
    fields.Resize(257);
    if (fields.VertexSize() != 257 || fields.heightfield.CellCount() != 257ull * 257ull)
        { std::printf("FAIL heightfield sized\n"); ++failures; }
    if (fields.flow.Width() != 257 || fields.accumulation.Width() != 257)
        { std::printf("FAIL flow/accum sized\n"); ++failures; }
    // Two distinct per-stratum families, both sized, both independently addressable: the
    // physical proportions the sims own and the visible weights the Mask stage owns (ARCH 7.2).
    for (int index = 0; index < Data::MapFields::stratumCount; ++index) {
        if (fields.materialProportions[index].CellCount() != 257ull * 257ull
         || fields.surfaceStratumWeights[index].CellCount() != 257ull * 257ull)
            { std::printf("FAIL stratum field %d sized\n", index); ++failures; break; }
    }
    fields.materialProportions[3].Fill(0.5f);
    if (fields.surfaceStratumWeights[3].Get(1, 1) != 0.0f)
        { std::printf("FAIL proportions and surface weights alias\n"); ++failures; }
    if (!fields.IsSized()) { std::printf("FAIL sized flag\n"); ++failures; }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
