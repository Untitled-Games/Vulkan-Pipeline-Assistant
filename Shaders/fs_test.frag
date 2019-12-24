#version 450

layout(location = 0) in vec4 inColour;

layout(location = 0) out vec4 outColour;

layout(set = 0, binding = 0) uniform Test00 {
	vec4 a;
	vec4 b;
	mat4 c;
	vec2 arr[40];
}; // 416 bytes

layout(set = 0, binding = 1) uniform Test01 {
	vec4 d;
}; // 16 bytes

layout(set = 1, binding = 2) buffer Test12 {
	vec4 arr2[];
};

layout(set = 2, binding = 1) uniform sampler2D Tex21;
layout(set = 2, binding = 2) uniform sampler3D Tex22;
layout(set = 2, binding = 3) uniform samplerCube Tex23;
layout(set = 2, binding = 4) uniform writeonly image2D Image24;

void main() 
{
	outColour = inColour;
}
