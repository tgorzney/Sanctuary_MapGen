#version 430 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(std430, binding = 0) buffer StratumData {
    float thicknesses[];
};

layout(std430, binding = 1) buffer PhysicsData {
    vec4 physics[]; // x=Hardness, y=Friction, z=Cohesion, w=CapacityMult
};

uniform int mapSize;
uniform int layerCount;

float getThickness(int layer, int x, int y) {
    if (x < 0 || x >= mapSize || y < 0 || y >= mapSize) return 0.0;
    return thicknesses[layer * mapSize * mapSize + y * mapSize + x];
}

void setThickness(int layer, int x, int y, float val) {
    thicknesses[layer * mapSize * mapSize + y * mapSize + x] = val;
}

float getTotalHeight(int x, int y) {
    float h = 0.0;
    for(int l=0; l<layerCount; ++l) {
        h += getThickness(l, x, y);
    }
    return h;
}

void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x < 1 || pos.x >= mapSize - 1 || pos.y < 1 || pos.y >= mapSize - 1) return;
    
    // Check top-down through layers for active stratums to slide
    for (int l = layerCount - 1; l >= 0; --l) {
        float thickness = getThickness(l, pos.x, pos.y);
        if (thickness > 0.001) {
            float maxSlope = physics[l].z; // Cohesion is z
            
            float h = getTotalHeight(pos.x, pos.y);
            int bestNX = pos.x;
            int bestNY = pos.y;
            float lowestH = h;
            
            // Check 4 neighbors
            int dx[4] = int[]( -1, 1, 0, 0 );
            int dy[4] = int[]( 0, 0, -1, 1 );
            
            for (int d = 0; d < 4; ++d) {
                float nh = getTotalHeight(pos.x + dx[d], pos.y + dy[d]);
                if (nh < lowestH) {
                    lowestH = nh;
                    bestNX = pos.x + dx[d];
                    bestNY = pos.y + dy[d];
                }
            }
            
            float diff = h - lowestH;
            if (diff > maxSlope) {
                float slideAmount = (diff - maxSlope) / 2.0;
                slideAmount = min(slideAmount, thickness);
                
                // Subtract from this cell
                setThickness(l, pos.x, pos.y, thickness - slideAmount);
                
                // Add to neighbor
                int nIdx = l * mapSize * mapSize + bestNY * mapSize + bestNX;
                thicknesses[nIdx] += slideAmount; // Stochastic race condition deposit
            }
        }
    }
}
