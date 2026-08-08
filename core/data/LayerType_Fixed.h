#pragma once
#include <string>
#include <vector>
#include "MarkerType_Rule.h"

namespace SanmapGen {

    struct ImportedMarkerLayer {
        std::string Name = "Imported Map Markers";
        bool Enabled = true;
        bool Locked = true; // Typically locked to prevent editing source map markers directly
        LayerType Type = LayerType::Fixed;
        std::vector<std::string> MarkerKeys;
    };

}