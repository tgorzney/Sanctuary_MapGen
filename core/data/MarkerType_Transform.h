#pragma once
#include <string>

namespace SanmapGen {

    enum MarkerSymmetry {
        Symmetry_None = 0,
        Symmetry_Point = 1 << 0,
        Symmetry_X = 1 << 1,
        Symmetry_Z = 1 << 2,
        Symmetry_XY = 1 << 3,
        Symmetry_XZ = 1 << 4,
        Symmetry_YZ = 1 << 5,
        Symmetry_XYZ = 1 << 6
    };

    struct MarkerTransform {
        float Position[3] = {0.0f, 0.0f, 0.0f};
        float Rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // x,y,z,w quaternion
        float Scale[3] = {1.0f, 1.0f, 1.0f};
        float Color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        int SymmetryId = 0; // The ID of the symmetry group if it's a mirrored clone
        
        bool SymmetryUseGlobal = true;
        int SymmetryMask = Symmetry_Point;
        
        // Keep track of what type this is (Spawn, Alloy, Plasma, etc.)
        std::string Type;
        // Allows customizing the JSON key for Spawn (e.g., "ARMY_1") or Alloys (e.g. "Mex 0")
        std::string CustomName = ""; 
        std::string IconOverride = ""; // Used to override the global visual icon for this specific marker
        
        // Differentiates procedurally generated markers from manual ones
        bool IsManual = false;
        // Used to highlight symmetrically forced invalid placements
        bool IsValid = true;
        // If the generating rule is disabled, keep it hidden but generate for clearance
        bool IsHidden = false;
    };

}
