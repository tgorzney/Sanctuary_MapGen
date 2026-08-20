#version 430 core
layout(local_size_x = WORKGROUP_SIZE) in;
layout(std430, binding = 0) buffer Data { float values[]; };
uniform int elementCount;
void main() {
    uint index = gl_GlobalInvocationID.x;
    if (index < uint(elementCount)) values[index] *= 2.0;
}
