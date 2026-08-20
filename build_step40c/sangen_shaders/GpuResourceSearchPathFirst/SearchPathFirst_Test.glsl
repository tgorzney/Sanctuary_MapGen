#version 430 core
layout(local_size_x = 1) in;
layout(std430, binding = 0) buffer Data { float values[]; };
void main() { values[0] = 1.0; }
