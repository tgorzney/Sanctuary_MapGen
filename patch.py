import os

with open('core/gen/Gen_FlowAndAccumulation.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Scalar loop injection
scalar_find = '''float noiseImpact = variance * (1.0f - std::min(1.0f, drop * adherence));'''
scalar_replace = '''float incomingVelScalar = flowPtr[y * vertSize + px];
                                                        float dynVar = variance + (incomingVelScalar * 0.5f);
                                                        float noiseImpact = dynVar * (1.0f - std::min(1.0f, drop * adherence));'''
content = content.replace(scalar_find, scalar_replace)

# 2. AVX2 loop injection
avx2_find = '''__m256 ad = _mm256_mul_ps(drop, _mm256_set1_ps(adherence));
                                            __m256 clampAd = _mm256_min_ps(_mm256_set1_ps(1.0f), _mm256_max_ps(_mm256_setzero_ps(), ad));
                                            __m256 noiseImpact = _mm256_mul_ps(_mm256_set1_ps(variance), _mm256_sub_ps(_mm256_set1_ps(1.0f), clampAd));'''
avx2_replace = '''__m256 incomingVelVec = _mm256_loadu_ps(&flowPtr[y * vertSize + x]);
                                            __m256 dynVar = _mm256_add_ps(_mm256_set1_ps(variance), _mm256_mul_ps(incomingVelVec, _mm256_set1_ps(0.5f)));
                                            __m256 ad = _mm256_mul_ps(drop, _mm256_set1_ps(adherence));
                                            __m256 clampAd = _mm256_min_ps(_mm256_set1_ps(1.0f), _mm256_max_ps(_mm256_setzero_ps(), ad));
                                            __m256 noiseImpact = _mm256_mul_ps(dynVar, _mm256_sub_ps(_mm256_set1_ps(1.0f), clampAd));'''
content = content.replace(avx2_find, avx2_replace)

with open('core/gen/Gen_FlowAndAccumulation.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print('Patched!')
