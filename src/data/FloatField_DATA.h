// FloatField_DATA.h — 2D contiguous float grid (row-major, SoA-friendly).
// Layer: DATA. The core computed-field container every PROC stage writes into
// (heightfield, masks, flow, accumulation). Replaces the old Mask2D/FloatMask.
// Plain data + accessors; no behavior, no GPU handles (ARCH §3).
#pragma once
#include <vector>
#include <cstddef>

namespace SanmapGen {
namespace Data {

class FloatField {
public:
    FloatField() : fieldWidth(0), fieldHeight(0) {}
    FloatField(int width, int height, float fillValue = 0.0f) { Resize(width, height, fillValue); }

    void Resize(int width, int height, float fillValue = 0.0f) {
        fieldWidth = width < 0 ? 0 : width;
        fieldHeight = height < 0 ? 0 : height;
        values.assign(static_cast<std::size_t>(fieldWidth) * fieldHeight, fillValue);
    }

    int Width() const { return fieldWidth; }
    int Height() const { return fieldHeight; }
    std::size_t CellCount() const { return values.size(); }
    bool IsEmpty() const { return values.empty(); }

    float Get(int x, int y) const { return values[static_cast<std::size_t>(y) * fieldWidth + x]; }
    void Set(int x, int y, float value) { values[static_cast<std::size_t>(y) * fieldWidth + x] = value; }
    float& At(int x, int y) { return values[static_cast<std::size_t>(y) * fieldWidth + x]; }
    const float& At(int x, int y) const { return values[static_cast<std::size_t>(y) * fieldWidth + x]; }

    float* Data() { return values.data(); }
    const float* Data() const { return values.data(); }

    void Fill(float value) { for (float& cell : values) cell = value; }

    // Bilinear sample in cell coordinates; sample position is clamped to the grid.
    float SampleBilinear(float sampleX, float sampleY) const {
        if (fieldWidth < 1 || fieldHeight < 1) return 0.0f;
        if (sampleX < 0.0f) sampleX = 0.0f;
        if (sampleY < 0.0f) sampleY = 0.0f;
        float maxX = static_cast<float>(fieldWidth - 1);
        float maxY = static_cast<float>(fieldHeight - 1);
        if (sampleX > maxX) sampleX = maxX;
        if (sampleY > maxY) sampleY = maxY;
        int lowX = static_cast<int>(sampleX);
        int lowY = static_cast<int>(sampleY);
        int highX = lowX < fieldWidth - 1 ? lowX + 1 : lowX;
        int highY = lowY < fieldHeight - 1 ? lowY + 1 : lowY;
        float fractionX = sampleX - static_cast<float>(lowX);
        float fractionY = sampleY - static_cast<float>(lowY);
        float top = Get(lowX, lowY) * (1.0f - fractionX) + Get(highX, lowY) * fractionX;
        float bottom = Get(lowX, highY) * (1.0f - fractionX) + Get(highX, highY) * fractionX;
        return top * (1.0f - fractionY) + bottom * fractionY;
    }

private:
    int fieldWidth;
    int fieldHeight;
    std::vector<float> values;
};

} // namespace Data
} // namespace SanmapGen
