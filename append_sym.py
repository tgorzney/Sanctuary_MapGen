import os

code = """
void TerrainGenerator::CalculateMarkerSymmetryGroups(GenerationParams& params) {
    // Reset all symmetry IDs
    for (auto& [key, marker] : params.MarkersList) {
        marker.SymmetryId = 0;
    }
    
    float mapSize = static_cast<float>(params.MapSize);
    uint32_t nextSymId = 1;
    
    std::vector<std::string> unprocessed;
    for (const auto& [key, marker] : params.MarkersList) {
        unprocessed.push_back(key);
    }
    
    while (!unprocessed.empty()) {
        std::string currentKey = unprocessed.front();
        unprocessed.erase(unprocessed.begin());
        
        auto& currentMarker = params.MarkersList[currentKey];
        if (currentMarker.SymmetryId != 0) continue;
        
        int mask = currentMarker.SymmetryUseGlobal ? params.GlobalSymmetryMask : currentMarker.SymmetryMask;
        if (mask == 0) continue;
        
        std::vector<std::string> group;
        group.push_back(currentKey);
        
        std::vector<std::pair<float, float>> expectedReflections;
        float x = currentMarker.Position[0];
        float z = currentMarker.Position[2];
        
        if (mask & Symmetry_Point) {
            expectedReflections.push_back({mapSize - x, mapSize - z});
        }
        if (mask & Symmetry_X) {
            expectedReflections.push_back({mapSize - x, z});
        }
        if (mask & Symmetry_Z) {
            expectedReflections.push_back({x, mapSize - z});
        }
        if (mask & Symmetry_XY) {
            expectedReflections.push_back({mapSize - z, mapSize - x});
        }
        if (mask & Symmetry_Radial) {
            expectedReflections.push_back({mapSize - x, mapSize - z});
            expectedReflections.push_back({mapSize - z, x});
            expectedReflections.push_back({z, mapSize - x});
        }
        
        float tolerance = params.SnapImperfectSymmetry ? 5.0f : 1.0f;
        
        for (auto expected : expectedReflections) {
            std::string bestMatch = "";
            float bestDist = tolerance * tolerance;
            
            for (auto it = unprocessed.begin(); it != unprocessed.end(); ++it) {
                auto& otherMarker = params.MarkersList[*it];
                if (otherMarker.Type != currentMarker.Type) continue;
                
                float dx = otherMarker.Position[0] - expected.first;
                float dz = otherMarker.Position[2] - expected.second;
                float distSq = dx*dx + dz*dz;
                
                if (distSq < bestDist) {
                    bestDist = distSq;
                    bestMatch = *it;
                }
            }
            
            if (!bestMatch.empty()) {
                group.push_back(bestMatch);
                unprocessed.erase(std::remove(unprocessed.begin(), unprocessed.end(), bestMatch), unprocessed.end());
                
                if (params.SnapImperfectSymmetry) {
                    auto& matchMarker = params.MarkersList[bestMatch];
                    matchMarker.Position[0] = expected.first;
                    matchMarker.Position[2] = expected.second;
                    matchMarker.Position[1] = currentMarker.Position[1];
                }
            }
        }
        
        if (group.size() > 1) {
            for (const auto& key : group) {
                params.MarkersList[key].SymmetryId = nextSymId;
            }
            nextSymId++;
        }
    }
}
"""

with open("core/TerrainGenerator.cpp", "r", encoding="utf-8") as f:
    content = f.read()

import re
# Replace the final closing namespace with our code + the namespace closure
content = re.sub(r'\}\s*//\s*namespace SanmapGen\s*$', code + '\n} // namespace SanmapGen\n', content)

with open("core/TerrainGenerator.cpp", "w", encoding="utf-8") as f:
    f.write(content)
