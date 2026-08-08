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

    struct FlowSettings {
            float Precipitation = 1.0f;
            int Iterations = 50;
            bool UseGPU = false; // Toggle CPU vs GPU
            GradientSettings Gradient = {
                "Default", 
                {
                    {0.0f, {0.0f, 0.0f, 0.2f, 1.0f}}, 
                    {50.0f, {0.0f, 0.4f, 0.8f, 1.0f}}, 
                    {100.0f, {0.0f, 1.0f, 1.0f, 1.0f}}
                },
                true
            };
        };

}
