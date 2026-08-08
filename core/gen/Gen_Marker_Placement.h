#pragma once
#include "../Parameters.h"

namespace SanmapGen {
    class Gen_Marker_Placement {
    public:
        // Processes marker symmetry groups to ensure symmetric pairs are perfectly aligned
        static void CalculateMarkerSymmetryGroups(GenerationParams& params);
    };
}
