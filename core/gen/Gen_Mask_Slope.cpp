#include "Gen_Mask_Slope.h"
#include "../math/Sanmath_SIMD.h"
#include <cmath>
#include <omp.h>
#include <immintrin.h>

namespace SanmapGen {

    void Gen_Mask_Slope::GenerateSlopeMap(const FloatMask& heightMap, FloatMask& outSlopeMap, bool bUseEngineParityMath, GenerationResult* result) {
        int vertSize = heightMap.GetWidth();
        outSlopeMap.Resize(vertSize, vertSize, 0.0f);
        
        const float* inData = heightMap.GetDataPtr();
        float* outData = outSlopeMap.GetMutableDataPtr();
        
        float globalMinSlope = 99999.0f;
        float globalMaxSlope = -99999.0f;
        
        // Use raw pointers and OpenMP for God-Tier performance
        #pragma omp parallel
        {
            float localMin = 99999.0f;
            float localMax = -99999.0f;
            
            #pragma omp for nowait
            for (int y = 1; y < vertSize - 1; ++y) {
                int rowOffset = y * vertSize;
                int prevRowOffset = (y - 1) * vertSize;
                int nextRowOffset = (y + 1) * vertSize;
                
                int x = 1;
                
                if (!bUseEngineParityMath) {
                    __m256 minVec = _mm256_set1_ps(99999.0f);
                    __m256 maxVec = _mm256_set1_ps(-99999.0f);
                    __m256 halfVec = _mm256_set1_ps(0.5f);
                    
                    for (; x <= vertSize - 1 - 8; x += 8) {
                        // Load adjacent values
                        __m256 right = _mm256_loadu_ps(inData + rowOffset + x + 1);
                        __m256 left = _mm256_loadu_ps(inData + rowOffset + x - 1);
                        __m256 bottom = _mm256_loadu_ps(inData + nextRowOffset + x);
                        __m256 top = _mm256_loadu_ps(inData + prevRowOffset + x);
                        
                        __m256 dx = _mm256_mul_ps(_mm256_sub_ps(right, left), halfVec);
                        __m256 dy = _mm256_mul_ps(_mm256_sub_ps(bottom, top), halfVec);
                        
                        __m256 gradSq = _mm256_add_ps(_mm256_mul_ps(dx, dx), _mm256_mul_ps(dy, dy));
                        
                        _mm256_storeu_ps(outData + rowOffset + x, gradSq);
                        
                        minVec = _mm256_min_ps(minVec, gradSq);
                        maxVec = _mm256_max_ps(maxVec, gradSq);
                    }
                    
                    float minArr[8];
                    float maxArr[8];
                    _mm256_storeu_ps(minArr, minVec);
                    _mm256_storeu_ps(maxArr, maxVec);
                    for(int i=0; i<8; ++i) {
                        if (minArr[i] < localMin) localMin = minArr[i];
                        if (maxArr[i] > localMax) localMax = maxArr[i];
                    }
                }
                
                for (; x < vertSize - 1; ++x) {
                    float dx = (inData[rowOffset + x + 1] - inData[rowOffset + x - 1]) * 0.5f;
                    float dy = (inData[nextRowOffset + x] - inData[prevRowOffset + x]) * 0.5f;
                    
                    float val;
                    if (bUseEngineParityMath) {
                        float lenSq = dx * dx + dy * dy + 1.0f;
                        float len = std::sqrt(lenSq);
                        float dotProduct = 1.0f / len; 
                        val = std::acos(dotProduct) * (180.0f / 3.14159265f);
                    } else {
                        val = dx * dx + dy * dy;
                    }
                    
                    outData[rowOffset + x] = val;
                    if (val < localMin) localMin = val;
                    if (val > localMax) localMax = val;
                }
            }
            
            #pragma omp critical
            {
                if (localMin < globalMinSlope) globalMinSlope = localMin;
                if (localMax > globalMaxSlope) globalMaxSlope = localMax;
            }
        }
        
        // Edge cases (x=0, x=vertSize-1, y=0, y=vertSize-1) remain 0.0f and can be considered min slope
        if (globalMinSlope > 0.0f && vertSize > 2) {
            globalMinSlope = 0.0f;
        }
        
        if (result) {
            if (bUseEngineParityMath) {
                // Values are already in degrees
                if (globalMinSlope < 0.0f) globalMinSlope = 0.0f;
                result->MapMinSlope = globalMinSlope;
                result->MapMaxSlope = globalMaxSlope;
            } else {
                // Convert gradient squared back to angles (in degrees) for the UI mapping
                if (globalMinSlope < 0.0f) globalMinSlope = 0.0f; // safety
                if (globalMaxSlope < 0.0f) globalMaxSlope = 0.0f;
                
                result->MapMinSlope = std::atan(std::sqrt(globalMinSlope)) * (180.0f / 3.14159265f);
                result->MapMaxSlope = std::atan(std::sqrt(globalMaxSlope)) * (180.0f / 3.14159265f);
            }
        }
    }

}
