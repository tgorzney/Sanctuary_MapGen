#pragma once

#include <vector>
#include <stdexcept>
#include <cstdint>

namespace SanmapGen {

    // Base class template for 2D masks used in procedural generation
    template <typename T>
    class Mask2D {
    protected:
        int width;
        int height;
        std::vector<T> data;

    public:
        Mask2D(int w, int h, T initialValue) : width(w), height(h), data(w * h, initialValue) {}
        virtual ~Mask2D() = default;

        int GetWidth() const { return width; }
        int GetHeight() const { return height; }

        T Get(int x, int y) const {
            if (x < 0 || x >= width || y < 0 || y >= height) return T(); // Return default if out of bounds
            return data[y * width + x];
        }

        void Set(int x, int y, T value) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                data[y * width + x] = value;
            }
        }
        
        // Expose raw data pointer for fast encoding (e.g. stb_image)
        const T* GetDataPtr() const { return data.data(); }
        T* GetMutableDataPtr() { return data.data(); }
    };

    // FloatMask used for Heightmaps and Probability calculations
    class FloatMask : public Mask2D<float> {
    public:
        FloatMask(int w, int h, float initialValue = 0.0f) : Mask2D<float>(w, h, initialValue) {}

        void Add(const FloatMask& other);
        void Multiply(float scalar);
        void Blur(int radius);
        void Clamp(float minVal, float maxVal);
        void Normalize();
    };

    // BooleanMask used for binary logic (water/land, passability)
    class BooleanMask : public Mask2D<uint8_t> {
    public:
        BooleanMask(int w, int h, bool initialValue = false) : Mask2D<uint8_t>(w, h, initialValue ? 1 : 0) {}

        void Invert();
        void Expand(int radius);
        void Shrink(int radius);
    };

} // namespace SanmapGen
