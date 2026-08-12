#include <iostream>
#include <cmath>

int main() {
    float hL = 0.5f;
    float hR = 0.5f + (0.1f); // 10% change between pixels
    float terrainMaxHeight = 128.0f;
    float cellSize = 1.0f;
    
    float dx = (hR - hL) * 0.5f * terrainMaxHeight / cellSize;
    float dy = 0.0f;
    
    float slopeDegrees = std::acos(1.0f / std::sqrt(dx * dx + dy * dy + 1.0f)) * 57.2957795131f;
    std::cout << "dx: " << dx << " slope: " << slopeDegrees << std::endl;
    return 0;
}
