#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
    struct GradientStop {
        float Location = 0.0f; // 0.0 to 100.0 (or mapped to degrees 0-90)
        float Color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        
        bool operator<(const GradientStop& other) const {
            return Location < other.Location;
        }
    };

    struct GradientSettings {
        std::string Name = "New Setting";
        std::vector<GradientStop> Stops;
        bool SmoothInterpolation = true;
    };

}
