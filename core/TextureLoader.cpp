#include "TextureLoader.h"
#include "miniz.h"
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <GL/gl.h>

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <unordered_map>
#include <algorithm>

// Define missing OpenGL compressed texture constants
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

typedef void (APIENTRY * PFNGLCOMPRESSEDTEXIMAGE2DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
static PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D_PTR = nullptr;

// Convert DXT1 to RGBA and colorkey pure black to transparent
void ConvertDXT1ToRGBAWithBlackKey(const uint8_t* dxt1Data, int width, int height, std::vector<uint8_t>& outRgba) {
    outRgba.resize(width * height * 4);
    auto unpackRGB565 = [](uint16_t color, uint8_t& r, uint8_t& g, uint8_t& b) {
        r = ((color >> 11) & 0x1F) * 255 / 31;
        g = ((color >> 5) & 0x3F) * 255 / 63;
        b = (color & 0x1F) * 255 / 31;
    };
    int blockCountX = (width + 3) / 4;
    int blockCountY = (height + 3) / 4;
    const uint8_t* block = dxt1Data;
    for (int by = 0; by < blockCountY; by++) {
        for (int bx = 0; bx < blockCountX; bx++) {
            uint16_t c0 = *(uint16_t*)(block);
            uint16_t c1 = *(uint16_t*)(block + 2);
            uint32_t bits = *(uint32_t*)(block + 4);
            block += 8;
            uint8_t r[4], g[4], b[4];
            unpackRGB565(c0, r[0], g[0], b[0]);
            unpackRGB565(c1, r[1], g[1], b[1]);
            if (c0 > c1) {
                r[2] = (2 * r[0] + r[1]) / 3; g[2] = (2 * g[0] + g[1]) / 3; b[2] = (2 * b[0] + b[1]) / 3;
                r[3] = (r[0] + 2 * r[1]) / 3; g[3] = (g[0] + 2 * g[1]) / 3; b[3] = (b[0] + 2 * b[1]) / 3;
            } else {
                r[2] = (r[0] + r[1]) / 2; g[2] = (g[0] + g[1]) / 2; b[2] = (b[0] + b[1]) / 2;
                r[3] = 0; g[3] = 0; b[3] = 0;
            }
            for (int py = 0; py < 4; py++) {
                for (int px = 0; px < 4; px++) {
                    int pixelX = bx * 4 + px;
                    int pixelY = by * 4 + py;
                    if (pixelX >= width || pixelY >= height) continue;
                    uint8_t code = (bits >> ((py * 4 + px) * 2)) & 3;
                    uint8_t pR = r[code], pG = g[code], pB = b[code], pA = 255;
                    if (pR < 10 && pG < 10 && pB < 10) pA = 0; // Colorkey near-black
                    if (c0 <= c1 && code == 3) pA = 0; // DXT1 transparency bit
                    int outIdx = (pixelY * width + pixelX) * 4;
                    outRgba[outIdx] = pR; outRgba[outIdx+1] = pG; outRgba[outIdx+2] = pB; outRgba[outIdx+3] = pA;
                }
            }
        }
    }
}

namespace SanmapGen {

// DDS Header Structs
struct DDS_PIXELFORMAT {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};

struct DDS_HEADER {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};

#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM_ARB
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB 0x8E8C
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB 0x8E8D
#endif
#ifndef GL_COMPRESSED_RED_RGTC1
#define GL_COMPRESSED_RED_RGTC1 0x8DBB
#endif
#ifndef GL_COMPRESSED_RG_RGTC2
#define GL_COMPRESSED_RG_RGTC2 0x8DBD
#endif

#define DXGI_FORMAT_BC4_UNORM 80
#define DXGI_FORMAT_BC5_UNORM 83
#define DXGI_FORMAT_BC7_UNORM 98
#define DXGI_FORMAT_BC7_UNORM_SRGB 99

struct DDS_HEADER_DXT10 {
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};

GLuint TextureLoader::LoadDDSFromArchive(const std::string& archivePath, const std::string& fileInsideArchive, std::string* debugOut, void* openZipArchive, int exactFileIndex) {
    if (!glCompressedTexImage2D_PTR) {
        glCompressedTexImage2D_PTR = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)wglGetProcAddress("glCompressedTexImage2D");
        if (!glCompressedTexImage2D_PTR) {
            if (debugOut) *debugOut += "  [DDS_ARCHIVE] ERROR: wglGetProcAddress failed for glCompressedTexImage2D\n";
            return 0;
        }
    }

    mz_zip_archive local_zip_archive = {};
    mz_zip_archive* zip_archive_ptr = nullptr;
    bool needsClose = false;
    
    if (openZipArchive) {
        zip_archive_ptr = static_cast<mz_zip_archive*>(openZipArchive);
    } else {
        if (!mz_zip_reader_init_file(&local_zip_archive, archivePath.c_str(), 0)) {
            if (debugOut) *debugOut += "  [DDS_ARCHIVE] ERROR: Failed to open archive (mz_zip_reader_init_file failed)\n";
            return 0;
        }
        zip_archive_ptr = &local_zip_archive;
        needsClose = true;
    }
    
    std::string searchName = fileInsideArchive;
    int file_index = exactFileIndex;

    if (file_index < 0) {
        file_index = mz_zip_reader_locate_file(zip_archive_ptr, searchName.c_str(), nullptr, 0);
    }
    
    if (file_index < 0) {
        if (debugOut) *debugOut += "  [DDS_ARCHIVE] ERROR: Could not find any file matching " + searchName + " inside archive.\n";
        if (needsClose) mz_zip_reader_end(zip_archive_ptr);
        return 0;
    }
    
    size_t uncomp_size = 0;
    void* p = mz_zip_reader_extract_to_heap(zip_archive_ptr, file_index, &uncomp_size, 0);
    if (needsClose) mz_zip_reader_end(zip_archive_ptr);
    
    if (!p) {
        if (debugOut) *debugOut += "  [DDS_ARCHIVE] ERROR: mz_zip_reader_extract_to_heap failed (corrupt zip?)\n";
        return 0;
    }
    
    const uint8_t* data = static_cast<const uint8_t*>(p);
    
    // Check DDS Magic
    if (uncomp_size < 128 || data[0] != 'D' || data[1] != 'D' || data[2] != 'S' || data[3] != ' ') {
        if (debugOut) *debugOut += "  [DDS_ARCHIVE] ERROR: File is not a valid DDS file (invalid magic header)\n";
        free(p);
        return 0;
    }
    
    DDS_HEADER header;
    memcpy(&header, data + 4, sizeof(DDS_HEADER));
    
    uint32_t width = header.dwWidth;
    uint32_t height = header.dwHeight;
    uint32_t format = 0;
    uint32_t blockSize = 16;
    
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_BGR
#define GL_BGR 0x80E0
#endif

    bool bIsCompressed = true;
    GLenum pixelFormat = GL_RGBA;
    const uint8_t* buffer = data + 128;
    size_t bufferSize = uncomp_size - 128;

    if (header.ddspf.dwFourCC == 0x31545844) { // DXT1
        format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        blockSize = 8;
    } else if (header.ddspf.dwFourCC == 0x33545844) { // DXT3
        format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    } else if (header.ddspf.dwFourCC == 0x35545844) { // DXT5
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    } else if (header.ddspf.dwFourCC == 0x30315844) { // 'DX10'
        if (uncomp_size < 128 + sizeof(DDS_HEADER_DXT10)) {
            if (debugOut) *debugOut += "  [DDS_ARCHIVE] ERROR: File too small for DX10 header\n";
            free(p); return 0;
        }
        DDS_HEADER_DXT10 dx10Header;
        memcpy(&dx10Header, data + 128, sizeof(DDS_HEADER_DXT10));
        buffer += sizeof(DDS_HEADER_DXT10);
        bufferSize -= sizeof(DDS_HEADER_DXT10);
        
        bIsCompressed = true;
        if (dx10Header.dxgiFormat == DXGI_FORMAT_BC7_UNORM || dx10Header.dxgiFormat == DXGI_FORMAT_BC7_UNORM_SRGB) {
            format = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            blockSize = 16;
        } else if (dx10Header.dxgiFormat == DXGI_FORMAT_BC5_UNORM) {
            format = GL_COMPRESSED_RG_RGTC2;
            blockSize = 16;
        } else if (dx10Header.dxgiFormat == DXGI_FORMAT_BC4_UNORM) {
            format = GL_COMPRESSED_RED_RGTC1;
            blockSize = 8;
        } else if (dx10Header.dxgiFormat == 87 || dx10Header.dxgiFormat == 28) {
            bIsCompressed = false;
            format = GL_RGBA;
            pixelFormat = (dx10Header.dxgiFormat == 87) ? GL_BGRA : GL_RGBA;
            header.ddspf.dwRGBBitCount = 32;
        } else {
            if (debugOut) *debugOut += "  [DDS_ARCHIVE] ERROR: Unsupported DXGI format: " + std::to_string(dx10Header.dxgiFormat) + "\n";
            free(p); return 0;
        }
    } else if (header.ddspf.dwFourCC == 0) {
        bIsCompressed = false;
        if (header.ddspf.dwFlags & 0x40) { // DDPF_RGB
            if (header.ddspf.dwRGBBitCount == 32) {
                format = GL_RGBA;
                pixelFormat = GL_BGRA;
            } else if (header.ddspf.dwRGBBitCount == 24) {
                format = GL_RGB;
                pixelFormat = GL_BGR;
            } else {
                if (debugOut) *debugOut += "  [DDS_ARCHIVE] ERROR: Unsupported uncompressed bit count: " + std::to_string(header.ddspf.dwRGBBitCount) + "\n";
                free(p);
                return 0;
            }
        } else {
            if (debugOut) *debugOut += "  [DDS_ARCHIVE] ERROR: FourCC is 0 but DDPF_RGB flag is missing\n";
            free(p);
            return 0;
        }
    } else {
        // Unsupported format
        if (debugOut) *debugOut += "  [DDS_ARCHIVE] ERROR: Unsupported DDS FourCC format: " + std::to_string(header.ddspf.dwFourCC) + " (Only DXT1/3/5 and Uncompressed are supported)\n";
        free(p);
        return 0;
    }
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Upload mip 0
    if (bIsCompressed) {
        uint32_t size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
        if (size > bufferSize) size = bufferSize;
        if (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
            std::vector<uint8_t> rgba;
            ConvertDXT1ToRGBAWithBlackKey(buffer, width, height, rgba);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        } else {
            glCompressedTexImage2D_PTR(GL_TEXTURE_2D, 0, format, width, height, 0, size, buffer);
            
            if (format == GL_COMPRESSED_RG_RGTC2) {
                std::vector<uint8_t> uncomp(width * height * 4);
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, uncomp.data());
                
                for (size_t i = 0; i < width * height; ++i) {
                    uint8_t r = uncomp[i*4 + 0]; // Marker shape
                    uint8_t g = uncomp[i*4 + 1]; // Glow shape
                    
                    // Map Marker to White, Glow to Cyan
                    uint8_t outR = r;
                    uint8_t outG = (std::max)(r, g);
                    uint8_t outB = (std::max)(r, g);
                    uint8_t outA = (std::max)(r, g);
                    
                    uncomp[i*4 + 0] = outR;
                    uncomp[i*4 + 1] = outG;
                    uncomp[i*4 + 2] = outB;
                    uncomp[i*4 + 3] = outA;
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, uncomp.data());
            }
        }
    } else {
        uint32_t bytesPerPixel = (header.ddspf.dwRGBBitCount / 8);
        uint32_t size = width * height * bytesPerPixel;
        if (size > bufferSize) size = bufferSize; // Safety (though glTexImage2D doesn't take size)
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, pixelFormat, GL_UNSIGNED_BYTE, buffer);
    }
    
    free(p);
    return textureID;
}

GLuint TextureLoader::LoadImageFromArchive(const std::string& archivePath, const std::string& fileInsideArchive, std::string* debugOut, void* openZipArchive, int exactFileIndex) {
    if (debugOut) *debugOut += "  [IMG_ARCHIVE] Checking " + archivePath + " for " + fileInsideArchive + "\n";
    
    mz_zip_archive local_zip_archive = {};
    mz_zip_archive* zip_archive_ptr = nullptr;
    bool needsClose = false;
    
    if (openZipArchive) {
        zip_archive_ptr = static_cast<mz_zip_archive*>(openZipArchive);
    } else {
        if (!mz_zip_reader_init_file(&local_zip_archive, archivePath.c_str(), 0)) {
            return 0;
        }
        zip_archive_ptr = &local_zip_archive;
        needsClose = true;
    }
    
    std::string searchName = fileInsideArchive;
    int file_index = exactFileIndex;

    if (file_index < 0) {
        file_index = mz_zip_reader_locate_file(zip_archive_ptr, searchName.c_str(), nullptr, 0);
    }
    
    if (file_index < 0) {
        if (debugOut) *debugOut += "  [IMAGE_ARCHIVE] ERROR: Could not find any file matching " + searchName + " inside archive.\n";
        if (needsClose) mz_zip_reader_end(zip_archive_ptr);
        return 0;
    }
    
    size_t uncomp_size = 0;
    void* p = mz_zip_reader_extract_to_heap(zip_archive_ptr, file_index, &uncomp_size, 0);
    if (needsClose) mz_zip_reader_end(zip_archive_ptr);
    
    if (!p) return 0;
    
    int w, h, channels;
    uint8_t* pixels = stbi_load_from_memory((const stbi_uc*)p, (int)uncomp_size, &w, &h, &channels, 4);
    free(p);
    
    if (!pixels) return 0;
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    stbi_image_free(pixels);
    return textureID;
}

GLuint TextureLoader::LoadImageFromFile(const std::string& filePath) {
    int w, h, channels;
    uint8_t* pixels = stbi_load(filePath.c_str(), &w, &h, &channels, 4);
    if (!pixels) return 0;
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    
    stbi_image_free(pixels);
    return textureID;
}

GLuint TextureLoader::LoadDDSFromFile(const std::string& filePath, std::string* debugOut) {
    if (debugOut) *debugOut += "  [DDS_FILE] Loading " + filePath + "\n";
    if (!glCompressedTexImage2D_PTR) {
        glCompressedTexImage2D_PTR = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)wglGetProcAddress("glCompressedTexImage2D");
        if (!glCompressedTexImage2D_PTR) {
            if (debugOut) *debugOut += "  [DDS_FILE] ERROR: glCompressedTexImage2D not available\n";
            return 0;
        }
    }
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        if (debugOut) *debugOut += "  [DDS_FILE] ERROR: Could not open file stream\n";
        return 0;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size < 128) {
        if (debugOut) *debugOut += "  [DDS_FILE] ERROR: File too small (" + std::to_string(size) + " bytes)\n";
        return 0;
    }
    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        if (debugOut) *debugOut += "  [DDS_FILE] ERROR: Could not read file contents\n";
        return 0;
    }
    
    if (data[0] != 'D' || data[1] != 'D' || data[2] != 'S' || data[3] != ' ') {
        if (debugOut) *debugOut += "  [DDS_FILE] ERROR: Invalid DDS magic header\n";
        return 0;
    }
    
    DDS_HEADER header;
    memcpy(&header, data.data() + 4, sizeof(DDS_HEADER));
    
    uint32_t width = header.dwWidth;
    uint32_t height = header.dwHeight;
    uint32_t format = 0;
    uint32_t blockSize = 16;
    
    const uint8_t* buffer = data.data() + 128;
    size_t bufferSize = size - 128;
    
    bool bIsCompressed = true;
    GLenum pixelFormat = GL_RGBA;
    if (header.ddspf.dwFourCC == 0x31545844) { format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; blockSize = 8; }
    else if (header.ddspf.dwFourCC == 0x33545844) { format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; }
    else if (header.ddspf.dwFourCC == 0x35545844) { format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; }
    else if (header.ddspf.dwFourCC == 0x30315844) { // 'DX10'
        if (size < 128 + sizeof(DDS_HEADER_DXT10)) {
            if (debugOut) *debugOut += "  [DDS_FILE] ERROR: File too small for DX10 header\n";
            return 0;
        }
        DDS_HEADER_DXT10 dx10Header;
        memcpy(&dx10Header, data.data() + 128, sizeof(DDS_HEADER_DXT10));
        buffer += sizeof(DDS_HEADER_DXT10);
        bufferSize -= sizeof(DDS_HEADER_DXT10);
        
        bIsCompressed = true;
        if (dx10Header.dxgiFormat == DXGI_FORMAT_BC7_UNORM || dx10Header.dxgiFormat == DXGI_FORMAT_BC7_UNORM_SRGB) {
            format = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            blockSize = 16;
        } else if (dx10Header.dxgiFormat == DXGI_FORMAT_BC5_UNORM) {
            format = GL_COMPRESSED_RG_RGTC2;
            blockSize = 16;
        } else if (dx10Header.dxgiFormat == DXGI_FORMAT_BC4_UNORM) {
            format = GL_COMPRESSED_RED_RGTC1;
            blockSize = 8;
        } else if (dx10Header.dxgiFormat == 87 /* DXGI_FORMAT_B8G8R8A8_UNORM */ || dx10Header.dxgiFormat == 28 /* DXGI_FORMAT_R8G8B8A8_UNORM */) {
            bIsCompressed = false;
            format = GL_RGBA;
            pixelFormat = (dx10Header.dxgiFormat == 87) ? GL_BGRA : GL_RGBA;
            header.ddspf.dwRGBBitCount = 32;
        } else {
            if (debugOut) *debugOut += "  [DDS_FILE] ERROR: Unsupported DXGI format: " + std::to_string(dx10Header.dxgiFormat) + "\n";
            return 0;
        }
    }
    else if (header.ddspf.dwFourCC == 0) {
        bIsCompressed = false;
        if (header.ddspf.dwFlags & 0x40) { // DDPF_RGB
            if (header.ddspf.dwRGBBitCount == 32) {
                format = GL_RGBA;
                pixelFormat = GL_BGRA;
            } else if (header.ddspf.dwRGBBitCount == 24) {
                format = GL_RGB;
                pixelFormat = GL_BGR;
            } else {
                if (debugOut) *debugOut += "  [DDS_FILE] ERROR: Unsupported uncompressed bit count: " + std::to_string(header.ddspf.dwRGBBitCount) + "\n";
                return 0;
            }
        } else {
            if (debugOut) *debugOut += "  [DDS_FILE] ERROR: FourCC is 0 but DDPF_RGB flag is missing\n";
            return 0;
        }
    } else {
        if (debugOut) *debugOut += "  [DDS_FILE] ERROR: Unsupported DDS FourCC format: " + std::to_string(header.ddspf.dwFourCC) + "\n";
        return 0;
    }
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    if (bIsCompressed) {
        uint32_t expectedSize = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
        if (expectedSize > bufferSize) expectedSize = bufferSize;
        if (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
            std::vector<uint8_t> rgba;
            ConvertDXT1ToRGBAWithBlackKey(buffer, width, height, rgba);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        } else {
            glCompressedTexImage2D_PTR(GL_TEXTURE_2D, 0, format, width, height, 0, expectedSize, buffer);
            
            if (format == GL_COMPRESSED_RG_RGTC2) {
                std::vector<uint8_t> uncomp(width * height * 4);
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, uncomp.data());
                
                for (size_t i = 0; i < width * height; ++i) {
                    uint8_t r = uncomp[i*4 + 0]; 
                    uint8_t g = uncomp[i*4 + 1]; 
                    
                    uint8_t outR = r;
                    uint8_t outG = (std::max)(r, g);
                    uint8_t outB = (std::max)(r, g);
                    uint8_t outA = (std::max)(r, g);
                    
                    uncomp[i*4 + 0] = outR;
                    uncomp[i*4 + 1] = outG;
                    uncomp[i*4 + 2] = outB;
                    uncomp[i*4 + 3] = outA;
                }
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, uncomp.data());
            }
        }
    } else {
        uint32_t bytesPerPixel = (header.ddspf.dwRGBBitCount / 8);
        uint32_t expectedSize = width * height * bytesPerPixel;
        if (expectedSize > bufferSize) expectedSize = bufferSize;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, pixelFormat, GL_UNSIGNED_BYTE, buffer);
    }
    
    return textureID;
}


std::vector<std::string> TextureLoader::ScanSanpackForMarkers(const std::string& archivePath, std::string* debugOut) {
    std::vector<std::string> markers;
    if (debugOut) *debugOut += "--- SCANNING FOR MARKERS ---\n";
    if (debugOut) *debugOut += "ArchivePath: " + archivePath + "\n";
    
    if (std::filesystem::is_directory(archivePath)) {
        if (debugOut) *debugOut += "Path is a directory. Searching for loose files...\n";
        std::string iconsPath = archivePath + "/UI/Sprites/Icons";
        if (debugOut) *debugOut += "Target Icons Path: " + iconsPath + "\n";
        
        if (std::filesystem::exists(iconsPath)) {
            if (debugOut) *debugOut += "Icons Path EXISTS.\n";
            auto ends_with = [](const std::string& str, const std::string& suffix) {
                return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
            };
            int filesFound = 0;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(iconsPath)) {
                if (!entry.is_regular_file()) continue;
                filesFound++;
                std::string name = entry.path().filename().string();
                if (ends_with(name, ".dds") || ends_with(name, ".png") || ends_with(name, ".jpg")) {
                    std::string lowerName = name;
                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                    std::string basename = name.substr(name.find_last_of('/') + 1);
                    std::string lowerBase = basename;
                    std::transform(lowerBase.begin(), lowerBase.end(), lowerBase.begin(), ::tolower);
                    if (lowerBase.find("icon_") == std::string::npos && lowerBase.find("_icon") == std::string::npos && lowerBase.find("_symbol") == std::string::npos) continue;
                    
                    std::string typeName = basename;
                    size_t dotPos = typeName.find_last_of('.');
                    if (dotPos != std::string::npos) typeName = typeName.substr(0, dotPos);
                    
                    // case insensitive erase of "icon" and "_" if they exist
                    std::string lowerType = typeName;
                    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
                    size_t idx = lowerType.find("icon");
                    if (idx != std::string::npos) { typeName.erase(idx, 4); lowerType.erase(idx, 4); }
                    idx = lowerType.find("_");
                    if (idx != std::string::npos) { typeName.erase(idx, 1); lowerType.erase(idx, 1); }
                    
                    if (!typeName.empty()) {
                        typeName[0] = toupper(typeName[0]);
                        if (std::find(markers.begin(), markers.end(), typeName) == markers.end()) {
                            markers.push_back(typeName);
                            if (debugOut) *debugOut += "  ACCEPTED: " + name + " -> Type: " + typeName + "\n";
                        }
                    }
                } else {
                    if (debugOut) *debugOut += "  Rejected: " + name + " (wrong extension)\n";
                }
            }
            if (debugOut) *debugOut += "Total files iterated: " + std::to_string(filesFound) + "\n";
        } else {
            if (debugOut) *debugOut += "Icons Path DOES NOT EXIST.\n";
        }
        if (debugOut) *debugOut += "Found " + std::to_string(markers.size()) + " markers in directory.\n";
        return markers;
    }
    
    if (debugOut) *debugOut += "Path is an archive (.sanpack/.zip). Searching inside zip...\n";
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, archivePath.c_str(), 0)) {
        return markers;
    }
    
    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    if (debugOut) *debugOut += "Zip contains " + std::to_string(numFiles) + " total files/folders.\n";
    
    int filesFound = 0;
    for (int i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
        
        std::string name = file_stat.m_filename;
        for (char& c : name) { if (c == '\\') c = '/'; }
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerName.find("ui/sprites/icons/") != std::string::npos) {
            filesFound++;
            size_t lastSlash = name.find_last_of('/');
            if (lastSlash != std::string::npos) {
                std::string basename = name.substr(lastSlash + 1);
                auto ends_with = [](const std::string& str, const std::string& suffix) {
                    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
                };
                if (ends_with(basename, ".dds") || ends_with(basename, ".png") || ends_with(basename, ".jpg")) {
                    std::string lowerBase = basename;
                    std::transform(lowerBase.begin(), lowerBase.end(), lowerBase.begin(), ::tolower);
                    if (lowerBase.find("icon_") == std::string::npos && lowerBase.find("_icon") == std::string::npos) continue;
                    
                    std::string typeName = basename;
                    size_t dotPos = typeName.find_last_of('.');
                    if (dotPos != std::string::npos) typeName = typeName.substr(0, dotPos);
                    
                    std::string lowerType = typeName;
                    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
                    size_t idx = lowerType.find("icon");
                    if (idx != std::string::npos) { typeName.erase(idx, 4); lowerType.erase(idx, 4); }
                    idx = lowerType.find("_");
                    if (idx != std::string::npos) { typeName.erase(idx, 1); lowerType.erase(idx, 1); }
                    
                    if (!typeName.empty()) {
                        typeName[0] = toupper(typeName[0]);
                        if (std::find(markers.begin(), markers.end(), typeName) == markers.end()) {
                            markers.push_back(typeName);
                            if (debugOut) *debugOut += "  ACCEPTED: " + basename + " -> Type: " + typeName + "\n";
                        }
                    }
                } else {
                    if (!basename.empty() && basename.find('.') != std::string::npos) {
                        if (debugOut) *debugOut += "  Rejected: " + basename + " (wrong extension)\n";
                    }
                }
            }
        }
    }
    mz_zip_reader_end(&zip_archive);
    
    if (debugOut) *debugOut += "Found " + std::to_string(markers.size()) + " markers in zip from " + std::to_string(filesFound) + " potential UI files.\n";
    return markers;
}
std::vector<std::string> TextureLoader::GetEnvironmentsFromSanpack(const std::string& zipPath) {
    std::vector<std::string> envs;
    if (zipPath.empty()) return envs;
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) return envs;
    
    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
        std::string fname = file_stat.m_filename;
        
        size_t stratumPos = fname.find("/Stratum/");
        if (stratumPos != std::string::npos) {
            std::string envFolder = fname.substr(0, stratumPos);
            if (std::find(envs.begin(), envs.end(), envFolder) == envs.end()) {
                envs.push_back(envFolder);
            }
        } else if (fname.find("Stratum/") == 0) {
            std::string envFolder = "Root";
            if (std::find(envs.begin(), envs.end(), envFolder) == envs.end()) {
                envs.push_back(envFolder);
            }
        }
    }
    mz_zip_reader_end(&zip_archive);
    return envs;
}

std::vector<std::string> TextureLoader::GetMaterialsFromSanpack(const std::string& zipPath, const std::string& env) {
    std::vector<std::string> mats;
    if (zipPath.empty() || env.empty()) return mats;
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) return mats;
    
    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
        std::string fname = file_stat.m_filename;
        if (fname.find(env) != std::string::npos && fname.find("Stratum") != std::string::npos && !mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
            size_t lastSlash = fname.find_last_of('/');
            std::string basename = (lastSlash != std::string::npos) ? fname.substr(lastSlash + 1) : fname;
            
            std::string basenameLower = basename;
            std::transform(basenameLower.begin(), basenameLower.end(), basenameLower.begin(), ::tolower);
            
            size_t underscore = basenameLower.rfind("_albedo");
            if (underscore == std::string::npos) underscore = basenameLower.rfind("_normal");
            if (underscore == std::string::npos) underscore = basenameLower.rfind("_mask");
            if (underscore != std::string::npos) {
                std::string mat = basename.substr(0, underscore);
                if (std::find(mats.begin(), mats.end(), mat) == mats.end()) {
                    mats.push_back(mat);
                }
            }
        }
    }
    mz_zip_reader_end(&zip_archive);
    return mats;
}

void TextureLoader::ScanSanpackForMaterial(const std::string& zipPath, const std::string& environmentTheme, const std::string& materialName, SanmapGen::StratumSettings& stratum) {
    if (zipPath.empty() || materialName.empty()) return;
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) return;
    
    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    
    std::string materialNameLower = materialName;
    std::transform(materialNameLower.begin(), materialNameLower.end(), materialNameLower.begin(), ::tolower);
    
    for (int i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;
        
        std::string fname = file_stat.m_filename;
        std::string fnameLower = fname;
        std::transform(fnameLower.begin(), fnameLower.end(), fnameLower.begin(), ::tolower);
        
        if (fname.find(environmentTheme) != std::string::npos && fname.find("Stratum") != std::string::npos && fnameLower.find(materialNameLower) != std::string::npos) {
            if (fnameLower.find("_albedo") != std::string::npos) {
                stratum.albedo.path = fname;
            } else if (fnameLower.find("_normal") != std::string::npos) {
                stratum.normal.path = fname;
            } else if (fnameLower.find("_mask") != std::string::npos) {
                stratum.mask.path = fname;
            }
        }
    }
    mz_zip_reader_end(&zip_archive);
}

std::queue<AsyncTextureRequest> AsyncTextureManager_RequestQueue;
std::queue<AsyncTextureResult> AsyncTextureManager_ReadyQueue;
std::mutex AsyncTextureManager_QueueMutex;
std::condition_variable AsyncTextureManager_QueueCond;
std::thread AsyncTextureManager_WorkerThread;
bool AsyncTextureManager_bIsRunning = false;
mz_zip_archive AsyncTextureManager_UIArchive;
bool AsyncTextureManager_bArchiveOpen = false;
std::unordered_map<std::string, bool> AsyncTextureManager_RequestedKeys;
std::string AsyncTextureManager_GamedataPath;

void AsyncTextureManagerWorkerLoop() {
    while (AsyncTextureManager_bIsRunning) {
        AsyncTextureRequest req;
        {
            std::unique_lock<std::mutex> lock(AsyncTextureManager_QueueMutex);
            AsyncTextureManager_QueueCond.wait(lock, [] { 
                return !AsyncTextureManager_RequestQueue.empty() || !AsyncTextureManager_bIsRunning; 
            });
            
            if (!AsyncTextureManager_bIsRunning && AsyncTextureManager_RequestQueue.empty()) break;
            
            req = AsyncTextureManager_RequestQueue.front();
            AsyncTextureManager_RequestQueue.pop();
        }
        
        AsyncTextureResult res;
        res.cacheKey = req.cacheKey;
        res.success = false;
        
        if (AsyncTextureManager_bArchiveOpen) {
            std::string searchName = req.typeName;
            std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);
            
            std::string candidates[2] = {
                "UI/Sprites/Icons/Units/" + searchName + "_icon.dds",
                "UI/Sprites/Icons/Units/" + searchName + ".dds"
            };
            
            int file_index = -1;
            for (int i = 0; i < 2; ++i) {
                file_index = mz_zip_reader_locate_file(&AsyncTextureManager_UIArchive, candidates[i].c_str(), nullptr, 0);
                if (file_index >= 0) {
                    // Try case-insensitive just in case
                    if (file_index < 0) file_index = mz_zip_reader_locate_file(&AsyncTextureManager_UIArchive, candidates[i].c_str(), nullptr, 0x0200 /*MZ_ZIP_FLAG_IGNORE_PATH/CASE*/);
                    break;
                }
            }
            
            if (file_index >= 0) {
                size_t uncomp_size = 0;
                void* p = mz_zip_reader_extract_to_heap(&AsyncTextureManager_UIArchive, file_index, &uncomp_size, 0);
                if (p) {
                    const uint8_t* data = static_cast<const uint8_t*>(p);
                    if (uncomp_size >= 128 && data[0] == 'D' && data[1] == 'D' && data[2] == 'S' && data[3] == ' ') {
                        DDS_HEADER header;
                        memcpy(&header, data + 4, sizeof(DDS_HEADER));
                        res.width = header.dwWidth;
                        res.height = header.dwHeight;
                        
                        uint32_t blockSize = 16;
                        res.format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                        res.bIsCompressed = true;
                        res.pixelFormat = GL_RGBA;
                        
                        const uint8_t* buffer = data + 128;
                        size_t bufferSize = uncomp_size - 128;
                        
                        if (header.ddspf.dwFourCC == 0x31545844) { res.format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; blockSize = 8; }
                        else if (header.ddspf.dwFourCC == 0x33545844) { res.format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; }
                        else if (header.ddspf.dwFourCC == 0x35545844) { res.format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; }
                        else if (header.ddspf.dwFourCC == 0) {
                            res.bIsCompressed = false;
                            if (header.ddspf.dwRGBBitCount == 32) {
                                res.format = GL_RGBA;
                                res.pixelFormat = GL_BGRA;
                            } else if (header.ddspf.dwRGBBitCount == 24) {
                                res.format = GL_RGB;
                                res.pixelFormat = GL_BGR;
                            }
                        }
                        
                        if (res.bIsCompressed) {
                            uint32_t size = ((res.width + 3) / 4) * ((res.height + 3) / 4) * blockSize;
                            if (size > bufferSize) size = bufferSize;
                            
                            if (res.format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
                                ConvertDXT1ToRGBAWithBlackKey(buffer, res.width, res.height, res.buffer);
                                res.pixelFormat = GL_RGBA;
                                res.bIsCompressed = false; 
                            } else {
                                res.buffer.assign(buffer, buffer + size);
                            }
                        } else {
                            uint32_t bytesPerPixel = (header.ddspf.dwRGBBitCount / 8);
                            uint32_t size = res.width * res.height * bytesPerPixel;
                            if (size > bufferSize) size = bufferSize;
                            res.buffer.assign(buffer, buffer + size);
                        }
                        res.success = true;
                    }
                    free(p);
                }
            }
        }
        
        {
            std::lock_guard<std::mutex> lock(AsyncTextureManager_QueueMutex);
            AsyncTextureManager_ReadyQueue.push(res);
        }
    }
}

void AsyncTextureManager::Init(const std::string& gamedataPath) {
    if (AsyncTextureManager_bIsRunning) return;
    
    AsyncTextureManager_GamedataPath = gamedataPath;
    std::string uiPack = gamedataPath + "/UI.sanpack";
    
    memset(&AsyncTextureManager_UIArchive, 0, sizeof(AsyncTextureManager_UIArchive));
    AsyncTextureManager_bArchiveOpen = mz_zip_reader_init_file(&AsyncTextureManager_UIArchive, uiPack.c_str(), 0);
    
    AsyncTextureManager_bIsRunning = true;
    AsyncTextureManager_WorkerThread = std::thread(AsyncTextureManagerWorkerLoop);
}

void AsyncTextureManager::Shutdown() {
    if (!AsyncTextureManager_bIsRunning) return;
    
    {
        std::lock_guard<std::mutex> lock(AsyncTextureManager_QueueMutex);
        AsyncTextureManager_bIsRunning = false;
        AsyncTextureManager_QueueCond.notify_all();
    }
    
    if (AsyncTextureManager_WorkerThread.joinable()) {
        AsyncTextureManager_WorkerThread.join();
    }
    
    if (AsyncTextureManager_bArchiveOpen) {
        mz_zip_reader_end(&AsyncTextureManager_UIArchive);
        AsyncTextureManager_bArchiveOpen = false;
    }
}

void AsyncTextureManager::RequestUnitIcon(const std::string& typeName, const std::string& cacheKey) {
    std::lock_guard<std::mutex> lock(AsyncTextureManager_QueueMutex);
    if (AsyncTextureManager_RequestedKeys.find(cacheKey) == AsyncTextureManager_RequestedKeys.end()) {
        AsyncTextureManager_RequestedKeys[cacheKey] = true;
        AsyncTextureManager_RequestQueue.push({cacheKey, typeName});
        AsyncTextureManager_QueueCond.notify_one();
    }
}

void AsyncTextureManager::ProcessReadyQueue(SanmapGen::GenerationParams& params) {
    if (!glCompressedTexImage2D_PTR) {
        glCompressedTexImage2D_PTR = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)wglGetProcAddress("glCompressedTexImage2D");
    }

    std::lock_guard<std::mutex> lock(AsyncTextureManager_QueueMutex);
    while (!AsyncTextureManager_ReadyQueue.empty()) {
        auto res = AsyncTextureManager_ReadyQueue.front();
        AsyncTextureManager_ReadyQueue.pop();
        
        if (res.success && res.width > 0 && res.height > 0) {
            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            if (res.bIsCompressed && glCompressedTexImage2D_PTR) {
                glCompressedTexImage2D_PTR(GL_TEXTURE_2D, 0, res.format, res.width, res.height, 0, res.buffer.size(), res.buffer.data());
            } else {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res.width, res.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, res.buffer.data());
            }
            
            params.IconCache[res.cacheKey] = textureID;
        } else {
            params.IconCache[res.cacheKey] = 0;
        }
    }
}

} // namespace SanmapGen
