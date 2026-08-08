#pragma once
#include <string>
#include "MarkerType_Transform.h"

namespace SanmapGen {

    enum MarkerPriority {
        Priority_LargestArea = 0,
        Priority_SmallestArea = 1,
        Priority_LeastVariance = 2
    };
    
    enum MarkerGradientType {
        Gradient_None = 0,
        Gradient_CenterFocus = 1,
        Gradient_EdgeFocus = 2,
        Gradient_Torus = 3
    };

    struct MarkerRule {
        std::string Name = "New Marker";
        bool Enabled = true;
        std::string Type = "Alloy";
        std::string IconOverride = ""; // Leave empty to use Type default
        float Color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        
        // Filtering thresholds
        float MinSlope = 0.0f;
        float MaxSlope = 90.0f;
        float MinHeight = 0.0f;
        float MaxHeight = 128.0f;
        
        // Advanced Deterministic Placement
        bool RandomSelection = false;
        int Priority = Priority_LeastVariance;
        
        float AreaHeightRange = 0.5f; // Tolerance for height variance
        float AreaRadiusMin = 5.0f;
        bool CheckMaxRadius = false;
        float AreaRadiusMax = 50.0f;
        
        float ClearanceSpacing = 10.0f; // Minimum distance to other markers
        float MapEdgePadding = 0.0f;    // Distance from the edge of the map
        
        int FocusGradient = Gradient_None;
        float FocusGradientRadius = 250.0f;
        float FocusGradientStrength = 1.0f;
        float FocusGradientContrast = 1.0f;
        
        bool UseDensity = true;
        float Density = 0.02f;
        int Count = 4;
        bool UseAllPositions = false; // Overrides count and density to place at all valid spots
        
        bool SymmetryUseGlobal = true;
        int SymmetryMask = Symmetry_Point;
    };

}
