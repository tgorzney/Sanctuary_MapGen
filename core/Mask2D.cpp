#include "Mask2D.h"
#include <algorithm>

namespace SanmapGen {

    // --- FloatMask Implementation ---

    void FloatMask::Add(const FloatMask& other) {
        if (width != other.GetWidth() || height != other.GetHeight()) {
            throw std::invalid_argument("Mask dimensions must match for Add");
        }
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] += other.data[i];
        }
    }

    void FloatMask::Multiply(float scalar) {
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] *= scalar;
        }
    }

    void FloatMask::Blur(int radius) {
        // TODO: Implement a fast box blur or gaussian blur
    }

    void FloatMask::Clamp(float minVal, float maxVal) {
        for (size_t i = 0; i < data.size(); ++i) {
            if (data[i] < minVal) data[i] = minVal;
            if (data[i] > maxVal) data[i] = maxVal;
        }
    }

    void FloatMask::Normalize() {
        if (data.empty()) return;
        float minVal = data[0];
        float maxVal = data[0];
        for (float v : data) {
            if (v < minVal) minVal = v;
            if (v > maxVal) maxVal = v;
        }
        if (maxVal - minVal > 0.0001f) {
            float range = maxVal - minVal;
            for (float& v : data) {
                v = (v - minVal) / range;
            }
        }
    }

    // --- BooleanMask Implementation ---

    void BooleanMask::Invert() {
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] = (data[i] == 0) ? 1 : 0;
        }
    }

    void BooleanMask::Expand(int radius) {
        // TODO: Implement mathematical morphology dilation
    }

    void BooleanMask::Shrink(int radius) {
        // TODO: Implement mathematical morphology erosion
    }

} // namespace SanmapGen
