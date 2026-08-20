#version 430 core
// Mask_Merge_PROC.glsl — GPU twin of the merge half of Mask_Merge_PROC.h: the three
// ImportedMaskMode merges. Same expressions in the same order as the CPU header
// (Constitution §4). The bilinear resampler stays in Mask_PROC.glsl because it reads the
// stored-art buffer, which cannot cross a GLSL compilation-unit boundary.
// One compilation unit of the Mask program (linked, never #included). Scalar arguments only.

// MASKING_SPEC verbatim: Disabled keeps the procedural mask, ProceduralStart is additive, and
// StaticOverride replaces it with the stored art — which is therefore NOT slope-gated, because
// that mode is locked to what the artist shipped.
float mergeStoredMask(float proceduralWeight, float storedWeight, int mergeMode,
                      float maskMinimum, float maskMaximum) {
    if (mergeMode == MASK_MERGE_PROCEDURAL_START)
        return clamp(proceduralWeight + storedWeight, maskMinimum, maskMaximum);
    if (mergeMode == MASK_MERGE_STATIC_OVERRIDE)
        return clamp(storedWeight, maskMinimum, maskMaximum);
    return clamp(proceduralWeight, maskMinimum, maskMaximum);
}
