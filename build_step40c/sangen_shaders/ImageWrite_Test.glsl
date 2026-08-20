#version 430 core
layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba8, binding = 0) uniform writeonly image2D destinationImage;
uniform int surfaceWidth;
uniform int surfaceHeight;
void main() {
    ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
    if (texel.x >= surfaceWidth || texel.y >= surfaceHeight) return;
    imageStore(destinationImage, texel, vec4(float(texel.x) / 255.0, float(texel.y) / 255.0, 0.0, 1.0));
}
