#version 430 core
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer StratumData {
    float thicknesses[];
};

layout(std430, binding = 1) buffer PhysicsData {
    vec4 physics[]; // x=Hardness, y=Friction, z=Cohesion, w=CapacityMult
};

layout(std430, binding = 2) buffer SpawnPoints {
    vec2 spawns[];
};

uniform int mapSize;
uniform int layerCount;
uniform int maxLifetime;
uniform float gravity;
uniform float evaporationRate;
uniform int totalDroplets;



float getThickness(int layer, int x, int y) {
    return thicknesses[layer * mapSize * mapSize + y * mapSize + x];
}

void setThickness(int layer, int x, int y, float val) {
    thicknesses[layer * mapSize * mapSize + y * mapSize + x] = val;
}

void addThickness(int layer, int x, int y, float amount) {
    // Stochastic Approximation - no atomic locks! Just race conditions!
    int idx = layer * mapSize * mapSize + y * mapSize + x;
    thicknesses[idx] += amount;
}

float getTotalHeight(float x, float y, out float gradX, out float gradY) {
    int coordX = int(x);
    int coordY = int(y);
    float u = x - float(coordX);
    float v = y - float(coordY);

    int x0 = clamp(coordX, 0, mapSize - 1);
    int y0 = clamp(coordY, 0, mapSize - 1);
    int x1 = clamp(coordX + 1, 0, mapSize - 1);
    int y1 = clamp(coordY + 1, 0, mapSize - 1);

    float h00 = 0.0;
    float h10 = 0.0;
    float h01 = 0.0;
    float h11 = 0.0;
    
    for(int l = 0; l < layerCount; ++l) {
        h00 += getThickness(l, x0, y0);
        h10 += getThickness(l, x1, y0);
        h01 += getThickness(l, x0, y1);
        h11 += getThickness(l, x1, y1);
    }

    gradX = (h10 - h00) * (1.0 - v) + (h11 - h01) * v;
    gradY = (h01 - h00) * (1.0 - u) + (h11 - h10) * u;
    
    return h00 * (1.0 - u) * (1.0 - v) + h10 * u * (1.0 - v) + h01 * (1.0 - u) * v + h11 * u * v;
}

float getTotalHeightInt(int x, int y) {
    float h = 0.0;
    for(int l=0; l<layerCount; ++l) {
        h += getThickness(l, x, y);
    }
    return h;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= totalDroplets) return;
    
    float posX = spawns[gid].x;
    float posY = spawns[gid].y;
    
    float dirX = 0.0;
    float dirY = 0.0;
    float speed = 1.0;
    float water = 1.0;
    float sediment = 0.0;
    
    for(int life = 0; life < maxLifetime; ++life) {
        int nodeX = int(posX);
        int nodeY = int(posY);
        
        float h, gradX, gradY;
        h = getTotalHeight(posX, posY, gradX, gradY);
        
        // Find top layer
        int topLayerIdx = 0;
        vec4 topPhysics = vec4(0.2, 0.8, 0.5, 2.0); // Sand default
        
        for(int l = layerCount - 1; l >= 0; --l) {
            if(getThickness(l, nodeX, nodeY) > 0.0001) {
                topLayerIdx = l;
                topPhysics = physics[l];
                break;
            }
        }
        
        float pHardness = topPhysics.x;
        float pFriction = topPhysics.y;
        float pCohesion = topPhysics.z;
        float pCapacityMult = topPhysics.w;
        
        float inertia = 0.05 + (1.0 - pFriction) * 0.1;
        
        dirX = (dirX * inertia) - (gradX * (1.0 - inertia));
        dirY = (dirY * inertia) - (gradY * (1.0 - inertia));
        
        float len = sqrt(dirX*dirX + dirY*dirY);
        if (len != 0.0) { dirX /= len; dirY /= len; }
        
        posX += dirX;
        posY += dirY;
        
        if (dirX == 0.0 && dirY == 0.0 || posX < 1.0 || posX >= float(mapSize - 2) || posY < 1.0 || posY >= float(mapSize - 2)) {
            break;
        }
        
        float newH, dummyX, dummyY;
        newH = getTotalHeight(posX, posY, dummyX, dummyY);
        float deltaHeight = newH - h;
        
        float capacity = max(-deltaHeight * speed * water * 4.0 * pCapacityMult, 0.01);
        
        if (sediment > capacity || deltaHeight > 0.0) {
            // Deposit
            float amountToDeposit = (deltaHeight > 0.0) ? min(deltaHeight, sediment) : (sediment - capacity) * 0.3;
            sediment -= amountToDeposit;
            
            float u = posX - float(int(posX));
            float v = posY - float(int(posY));
            
            addThickness(topLayerIdx, nodeX, nodeY, amountToDeposit * (1.0 - u) * (1.0 - v));
            addThickness(topLayerIdx, nodeX+1, nodeY, amountToDeposit * u * (1.0 - v));
            addThickness(topLayerIdx, nodeX, nodeY+1, amountToDeposit * (1.0 - u) * v);
            addThickness(topLayerIdx, nodeX+1, nodeY+1, amountToDeposit * u * v);
            
        } else {
            // Erode
            float erosionRate = 0.3 * (1.0 - pHardness);
            float amountToErode = min((capacity - sediment) * erosionRate, -deltaHeight);
            
            if (amountToErode > 0.0) {
                sediment += amountToErode;
                
                float u = posX - float(int(posX));
                float v = posY - float(int(posY));
                
                float e00 = amountToErode * (1.0 - u) * (1.0 - v);
                float e10 = amountToErode * u * (1.0 - v);
                float e01 = amountToErode * (1.0 - u) * v;
                float e11 = amountToErode * u * v;
                
                // Subtract sequentially
                float rem00 = e00;
                for(int l = layerCount - 1; l >= 0 && rem00 > 0.0; --l) {
                    float th = getThickness(l, nodeX, nodeY);
                    float sub = min(th, rem00);
                    if (sub > 0.0) { setThickness(l, nodeX, nodeY, th - sub); rem00 -= sub; }
                }
                
                float rem10 = e10;
                for(int l = layerCount - 1; l >= 0 && rem10 > 0.0; --l) {
                    float th = getThickness(l, nodeX+1, nodeY);
                    float sub = min(th, rem10);
                    if (sub > 0.0) { setThickness(l, nodeX+1, nodeY, th - sub); rem10 -= sub; }
                }
                
                float rem01 = e01;
                for(int l = layerCount - 1; l >= 0 && rem01 > 0.0; --l) {
                    float th = getThickness(l, nodeX, nodeY+1);
                    float sub = min(th, rem01);
                    if (sub > 0.0) { setThickness(l, nodeX, nodeY+1, th - sub); rem01 -= sub; }
                }
                
                float rem11 = e11;
                for(int l = layerCount - 1; l >= 0 && rem11 > 0.0; --l) {
                    float th = getThickness(l, nodeX+1, nodeY+1);
                    float sub = min(th, rem11);
                    if (sub > 0.0) { setThickness(l, nodeX+1, nodeY+1, th - sub); rem11 -= sub; }
                }
            }
        }
        
        speed = sqrt(max(0.0, speed * speed + deltaHeight * gravity));
        water *= (1.0 - evaporationRate);
    }
}
