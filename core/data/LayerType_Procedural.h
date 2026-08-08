#pragma once
#include <string>
#include <vector>
#include "MarkerType_Rule.h"

namespace SanmapGen {

    enum class LayerType {
        Procedural,
        Manual,
        Fixed
    };

    struct ProceduralMarkerLayer {
        std::string Name = "New Procedural Layer";
        bool Enabled = true;
        bool Locked = false;
        std::vector<MarkerRule> Rules;
    };

}