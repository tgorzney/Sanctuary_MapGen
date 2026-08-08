#pragma once
#include <string>
#include <vector>
#include "MarkerType_Rule.h"

namespace SanmapGen {

    struct PlacedMarkerLayer {
        std::string Name = "New Placed Layer";
        bool Enabled = true;
        bool Locked = false;
        
        // Differentiate manually created layers from fixed imported ones
        LayerType Type = LayerType::Manual;
        
        // Instead of duplicating marker data, we just store their lookup keys
        std::vector<std::string> MarkerKeys;
    };

}