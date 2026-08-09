#version 430 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(std430, binding = 0) buffer StratumData {
    float thicknesses[];
};

layout(std430, binding = 1) buffer PhysicsData {
    vec4 physics[]; // x=Hardness, y=Friction, z=Cohesion, w=CapacityMult
};

uniform int mapSize;
uniform int layerCount;       // number of layers uploaded (= currentLayerIdx + 1)
uniform int currentLayerSlot; // index of the current layer within the flattened array
uniform int erodeBeneath;     // 1 = may slide layers below current, 0 = only current layer

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

    // Scan layers in scope (all up to currentLayerSlot if erodeBeneath, else just currentLayerSlot)
    int scanBottom = (erodeBeneath != 0) ? 0 : currentLayerSlot;

    for (int l = layerCount - 1; l >= scanBottom; --l) {
        if (physics[l].x < 0.0) continue; // not erodable
        float thickness = getThickness(l, pos.x, pos.y);
        if (thickness <= 0.001) continue;

        float maxSlope = physics[l].z; // Cohesion
        float h = getTotalHeight(pos.x, pos.y);

        float h_l = getTotalHeight(pos.x - 1, pos.y);
        float h_r = getTotalHeight(pos.x + 1, pos.y);
        float h_u = getTotalHeight(pos.x, pos.y - 1);
        float h_d = getTotalHeight(pos.x, pos.y + 1);

        float dh_l = max(0.0, h - h_l);
        float dh_r = max(0.0, h - h_r);
        float dh_u = max(0.0, h - h_u);
        float dh_d = max(0.0, h - h_d);

        float total_dh = dh_l + dh_r + dh_u + dh_d;

        float slideActive = (total_dh > maxSlope) ? 1.0 : 0.0;
        float slideAmount = min(thickness, (total_dh - maxSlope) / 2.0) * slideActive;

        float inv_total_dh = (total_dh > 0.00001) ? (1.0 / total_dh) : 0.0;

        float slip_l = slideAmount * (dh_l * inv_total_dh);
        float slip_r = slideAmount * (dh_r * inv_total_dh);
        float slip_u = slideAmount * (dh_u * inv_total_dh);
        float slip_d = slideAmount * (dh_d * inv_total_dh);

        float total_slip = slip_l + slip_r + slip_u + slip_d;

        setThickness(l, pos.x, pos.y, thickness - total_slip);

        int bL = l * mapSize * mapSize;
        if(slip_l > 0.0) thicknesses[bL + pos.y * mapSize + (pos.x - 1)] += slip_l;
        if(slip_r > 0.0) thicknesses[bL + pos.y * mapSize + (pos.x + 1)] += slip_r;
        if(slip_u > 0.0) thicknesses[bL + (pos.y - 1) * mapSize + pos.x] += slip_u;
        if(slip_d > 0.0) thicknesses[bL + (pos.y + 1) * mapSize + pos.x] += slip_d;
    }
}

