#pragma once
#include <vector>
#include <cmath>
#include <utility>
#include "../Mask2D.h"

namespace SanmapGen {
    namespace Math {
        inline std::pair<int, float> ScoreRadialClearance(const FloatMask& heightmap, int cx, int cy, float minHeight, float maxHeight, float heightTolerance, int maxSearchRadius, int minStartRadius = 1) {
            float centerH = heightmap.Get(cx, cy);
            if (centerH < minHeight || centerH > maxHeight) return {0, 0.0f};

            auto checkPerimeter = [&](int r) -> bool {
                int x = r;
                int y = 0;
                int err = 1 - x;
                float minH = centerH;
                float maxH = centerH;
                int w = heightmap.GetWidth();
                int h_map = heightmap.GetHeight();

                while (x >= y) {
                    int pts[8][2] = {
                        {cx + x, cy + y}, {cx + y, cy + x}, {cx - y, cy + x}, {cx - x, cy + y},
                        {cx - x, cy - y}, {cx - y, cy - x}, {cx + y, cy - x}, {cx + x, cy - y}
                    };
                    for (int i = 0; i < 8; i++) {
                        int px = pts[i][0];
                        int py = pts[i][1];
                        if (px < 0 || px >= w || py < 0 || py >= h_map) return false;
                        
                        float h = heightmap.Get(px, py);
                        if (h < minH) minH = h;
                        if (h > maxH) maxH = h;
                        if ((maxH - minH) > heightTolerance) return false;
                    }
                    y++;
                    if (err < 0) {
                        err += 2 * y + 1;
                    } else {
                        x--;
                        err += 2 * (y - x + 1);
                    }
                }
                return true;
            };

            int startR = minStartRadius;
            if (!checkPerimeter(startR)) return {0, 0.0f};

            int low = startR;
            int high = maxSearchRadius;
            int step = startR;

            while (true) {
                int nextR = low + step;
                if (nextR > high) {
                    high = maxSearchRadius;
                    break;
                }
                if (checkPerimeter(nextR)) {
                    low = nextR;
                    step *= 2;
                } else {
                    high = nextR - 1;
                    break;
                }
            }

            int bestR = low;
            while (low <= high) {
                int mid = low + (high - low) / 2;
                if (checkPerimeter(mid)) {
                    bestR = mid;
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }

            return {bestR, 0.0f}; // Variance simplified for speed
        }

        inline std::pair<int, float> ScoreRadialClearance_Stochastic(const FloatMask& heightmap, int cx, int cy, float minHeight, float maxHeight, float heightTolerance, int maxSearchRadius, int minStartRadius = 1, uint32_t seed = 12345) {
            float centerH = heightmap.Get(cx, cy);
            if (centerH < minHeight || centerH > maxHeight) return {0, 0.0f};

            auto checkPerimeterStochastic = [&](int r) -> bool {
                int w = heightmap.GetWidth();
                int h_map = heightmap.GetHeight();
                float minH = centerH;
                float maxH = centerH;
                
                // Fast pseudo-random utilizing the radius and seed
                uint32_t rSeed = seed ^ (r * 19349663) ^ (cx * 73856093) ^ (cy * 83492791);
                float randomAng = (float)(rSeed % 360) * (3.14159265f / 180.0f);
                
                float stepAngle = (2.0f * 3.14159265f) / 8.0f;
                for (int i = 0; i < 8; ++i) {
                    float ang = randomAng + stepAngle * i;
                    int px = cx + (int)(std::cos(ang) * r);
                    int py = cy + (int)(std::sin(ang) * r);
                    
                    if (px < 0 || px >= w || py < 0 || py >= h_map) return false;
                    
                    float h = heightmap.Get(px, py);
                    if (h < minH) minH = h;
                    if (h > maxH) maxH = h;
                    if ((maxH - minH) > heightTolerance) return false;
                }
                return true;
            };

            int startR = minStartRadius;
            if (!checkPerimeterStochastic(startR)) return {0, 0.0f};

            int low = startR;
            int high = maxSearchRadius;
            int step = startR;

            while (true) {
                int nextR = low + step;
                if (nextR > high) {
                    high = maxSearchRadius;
                    break;
                }
                if (checkPerimeterStochastic(nextR)) {
                    low = nextR;
                    step *= 2;
                } else {
                    high = nextR - 1;
                    break;
                }
            }

            int bestR = low;
            while (low <= high) {
                int mid = low + (high - low) / 2;
                if (checkPerimeterStochastic(mid)) {
                    bestR = mid;
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }

            return {bestR, 0.0f};
        }

        // Jump Flooding Algorithm (JFA) for O(1) Distance Field lookups
        // Returns a FloatMask containing the distance to the nearest invalid pixel.
        inline FloatMask ComputeJFADistanceField(const FloatMask& heightmap, float minHeight, float maxHeight, float heightTolerance, float maxRadius) {
            int w = heightmap.GetWidth();
            int h = heightmap.GetHeight();
            
            // JFA uses coordinate buffers. We store the nearest seed coordinates (X, Y).
            // Uninitialized seeds are (-1, -1)
            struct Coord { short x, y; };
            std::vector<Coord> buffer(w * h, {-1, -1});
            
            // 1. Seed Initialization
            #pragma omp parallel for
            for (int y = 1; y < h - 1; ++y) {
                for (int x = 1; x < w - 1; ++x) {
                    float ch = heightmap.Get(x, y);
                    bool invalid = false;
                    if (ch < minHeight || ch > maxHeight) {
                        invalid = true;
                    } else {
                        // Check local variance for steepness
                        float dx = (heightmap.Get(x+1, y) - heightmap.Get(x-1, y)) * 0.5f;
                        float dy = (heightmap.Get(x, y+1) - heightmap.Get(x, y-1)) * 0.5f;
                        float gradMag = std::abs(dx) + std::abs(dy); // Approximation
                        // If gradient is too high, it's an obstacle
                        if (gradMag > (heightTolerance * 0.5f)) invalid = true;
                    }
                    
                    if (invalid) {
                        buffer[y * w + x] = {(short)x, (short)y};
                    }
                }
            }
            
            // 2. Jump Flooding Passes
            std::vector<Coord> nextBuffer = buffer;
            int step = std::max(w, h) / 2;
            
            while (step >= 1) {
                #pragma omp parallel for
                for (int y = 0; y < h; ++y) {
                    for (int x = 0; x < w; ++x) {
                        Coord best = buffer[y * w + x];
                        float bestDistSq = 99999999.0f;
                        
                        if (best.x != -1) {
                            float dx = (float)(x - best.x);
                            float dy = (float)(y - best.y);
                            bestDistSq = dx*dx + dy*dy;
                        }
                        
                        for (int dy = -1; dy <= 1; ++dy) {
                            for (int dx = -1; dx <= 1; ++dx) {
                                if (dx == 0 && dy == 0) continue;
                                
                                int nx = x + dx * step;
                                int ny = y + dy * step;
                                
                                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                                    Coord seed = buffer[ny * w + nx];
                                    if (seed.x != -1) {
                                        float ddx = (float)(x - seed.x);
                                        float ddy = (float)(y - seed.y);
                                        float distSq = ddx*ddx + ddy*ddy;
                                        if (distSq < bestDistSq) {
                                            bestDistSq = distSq;
                                            best = seed;
                                        }
                                    }
                                }
                            }
                        }
                        
                        nextBuffer[y * w + x] = best;
                    }
                }
                buffer = nextBuffer;
                step /= 2;
            }
            
            // 3. Convert to FloatMask Distances
            FloatMask distanceField(w, h, 0.0f);
            
            #pragma omp parallel for
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    Coord seed = buffer[y * w + x];
                    if (seed.x != -1) {
                        float dx = (float)(x - seed.x);
                        float dy = (float)(y - seed.y);
                        float distSq = dx*dx + dy*dy;
                        distanceField.Set(x, y, std::sqrt(distSq));
                    } else {
                        distanceField.Set(x, y, maxRadius); // No obstacle found
                    }
                }
            }
            
            return distanceField;
        }
    }
}
